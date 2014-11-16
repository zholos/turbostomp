#include "sea.h"

#include "assets.h"
#include "effect.h"
#include "grid.h"
#include "mesh.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <set>
#include <utility>

using std::cout;
using std::count_if;
using std::exchange;
using std::make_tuple;
using std::move;
using std::list;
using std::prev;
using std::set;
using pgamecc::operator<<;


//
// Sprite
//

dloc
Sprite::location() const
{
    return island ? body.location() + dvec3(island->origin) : dormant_location;
}

Box
Sprite::bound() const
{
    auto l = location();
    double r = radius();
    ivec3 p0 = ivec3(glm::floor(l.p-r)),
          p1 = ivec3(glm::ceil(l.p+r));
    return Box::ranged(p0, p1);
}



//
// Island
//

Island::Island(Sea& sea, Box bound) :
    sea(sea),
    contact_joints(world),
    bound(bound),
    origin(bound.center())
{
    world.set_gravity(dvec3(0, -10, 0)); // TODO: this is for testing
}


void
Island::insert(unique_ptr<Sprite> owned)
{
    Sprite& sprite = *owned.get();
    assert(!sprite.island);
    ode::TranslatedBody body(world);
    body.set_location(sprite.dormant_location - dvec3(origin));
    sprite.base_active(this, move(body));
    sprites.push_back(move(owned));
    sprite.placed();
}


ode::Geom
Island::create_voxel(SBox s, Tile t)
{
    static const auto overlap = glm::translate(glm::dvec3(.5)) *
                                glm::scale(glm::dvec3(1.02)) *
                                glm::translate(glm::dvec3(-.5));
    static ode::TriMeshData shape_mesh[] = {
        Mesh(meshes["ramp.obj"], overlap),
        Mesh(meshes["corner1.obj"], overlap),
        Mesh(meshes["corner2.obj"], overlap)
    };

    if (t.shape()) {
        assert(s.size() == 1);
        auto shape = voxel_space.new_trimesh(shape_mesh[t.shape_mesh()-1]);
        shape.set_location(t.shape_loc() + dvec3(s.p0() - origin));
        return move(shape);
    } else {
        // prevent zero-width spaces between boxes
        // matches same parameter on tile meshes
        const double overlap = .01;

        auto box = voxel_space.new_box(dvec3(s.size()) + 2*overlap);
        box.set_location({
            s.center_dvec3() - overlap - dvec3(origin), dquat() });
        return move(box);
    }
}

void
Island::sync(const Grid& grid)
{
    Box new_bound{ivec3()};
    for (auto& sprite: sprites) {
        auto b = sprite->bound();
        if (new_bound.empty())
            new_bound = b;
        else
            new_bound |= b;

        // all keys from map before hint are guaranteed < key of tile
        auto hint = voxels.begin();

        grid.ctop().each_tile(b, [&] (SBox s, Tile t) {
            assert(t); // not empty, assumed by collision code
            int repeat = 0;
        again:
            assert(hint == voxels.begin() ||
                   Grid::cursor::coord_less(prev(hint)->first, s.p0()));
            if (hint == voxels.end())
            insert:
                hint = voxels.emplace_hint(hint, s.p0(),
                    Voxel{create_voxel(s, t), s, t, true});
            else if (hint->first == s.p0()) {
                Voxel& v = hint->second;
                // keep tile updated to avoid cursor lookup
                if (v.box.size() != s.size() || v.tile != t) {
                    v.geom = create_voxel(s, t);
                    v.box = s;
                    v.tile = t;
                }
                assert(v.box == s);
                v.seen = true;
            } else if (Grid::cursor::coord_less(s.p0(), hint->first))
                // this is more expensive and less likely to be true than then
                // == test above, so ordered here instead of as an alternative
                // condition for the insert clause
                goto insert;
            else {
                // short forward search before exhaustive search
                if (repeat++ < 3)
                    ++hint;
                else
                    hint = voxels.lower_bound(s.p0());
                    // now one of the other conditions will be met
                goto again;
            }
            ++hint;
        });
    }
    bound = new_bound;

    for (auto it = voxels.begin(); it != voxels.end();)
        if (it->second.seen) {
            it->second.seen = false;
            ++it;
        } else
            it = voxels.erase(it);
}


void
Island::tick(Grid& grid)
{
    for (auto& sprite: sprites)
        sprite->before_tick();


    // Handle collisions. Only collect information, don't mutate spaces. ODE
    // stores geoms in a linked list. Adding to the list during iteration might
    // work, removing won't.

    set<Sprite*> destroyed_sprites;
    list<ivec3> destroyed_voxels;
    // NOTE: smaller space last, otherwise ODE uses a swapped callback
    ode::collide(voxel_space, sprite_space,
                 [&] (const auto& voxel_geom, const auto& sprite_geom) {
        ode::contacts(sprite_geom, voxel_geom, [&] (ode::Contact contact) {
            Sprite& sprite = Sprite::find(sprite_geom.body());

            if (destroyed_sprites.count(&sprite))
                return;

            // voxel_geom should be a reference to Voxel::geom, not a temporary
            // GeomRef
            const ode::Geom& voxel_geom_member =
                static_cast<const ode::Geom&>(voxel_geom);
            Voxel& voxel = *const_cast<Voxel*>(
                reinterpret_cast<const Voxel*>(&voxel_geom_member));
            assert(&voxels.at(voxel.box.p0()).geom == &voxel_geom_member);

            if (!voxel.tile)
                return; // was destroyed in previous iteraton

            // Now figure out which size-1 box the collision was with. Typically
            // position will be just outside the box due to overlap.
            // TODO: maybe use normal to get correct rounding direction
            // TODO: reorder after collide_tile, since we don't need it unless
            // that edits the tile
            auto b = SBox{1} +
                glm::clamp(ivec3(glm::floor(contact.position())) + origin,
                           voxel.box.p0(), voxel.box.p1()-1);
            assert([&]{
                auto c = grid.ctop().find_smallest(b);
                return c.is_tile() && c.tile() == voxel.tile;
            }());

            Tile last_tile = voxel.tile;
            switch (sprite.collide(voxel.tile)) {
            case Sprite::interaction::hit:
                contact.set_mu(.5);
                contact.set_bounce(.5 * (sprite.bounciness() + .5));
                contact.set_contact_approx_1(); // possibly not to get stuck
                contact_joints.new_contact_joint(contact).attach(
                    &sprite.body, nullptr);
                break;
            case Sprite::interaction::destroyed:
                destroyed_sprites.insert(&sprite);
                break; // keep going, tile may also have been destroyed
            }

            if (last_tile != voxel.tile) { // changed by collide()
                grid.top().fill(b, voxel.tile);
                new BoxEffect(b);
                if (!voxel.tile)
                    destroyed_voxels.push_back(b.p0());
            }
        });
    });
    for (auto i: exchange(destroyed_voxels, {}))
        voxels.erase(i);
    for (auto i: exchange(destroyed_sprites, {}))
        // TODO: optimize
        sprites.remove_if([&] (auto& p) { return p.get() == i; });

    auto sprite_callback = [&] (const auto& sprite1_geom,
                                const auto& sprite2_geom)
    {
        ode::contacts(sprite1_geom, sprite2_geom,
                      [&] (ode::Contact contact) {
            Sprite& sprite1 = Sprite::find(sprite1_geom.body());
            Sprite& sprite2 = Sprite::find(sprite2_geom.body());
            assert(&sprite1 != &sprite2);
            if (destroyed_sprites.count(&sprite1) ||
                destroyed_sprites.count(&sprite2))
                return;

            auto i1 = sprite1.collide(sprite2);
            auto i2 = sprite2.collide(sprite1);

            // TODO: maybe allow one to collide and other to ignore
            if (i1 != Sprite::interaction::ignore &&
                i2 != Sprite::interaction::ignore) {
                contact.set_mu(.5);
                contact.set_bounce(.5 *
                    (sprite1.bounciness() + sprite2.bounciness()));
                contact.set_contact_approx_1(); // possibly not to get stuck
                contact_joints.new_contact_joint(contact).attach(
                    &sprite1_geom.body(), &sprite2_geom.body());
            }

            if (i1 == Sprite::interaction::destroyed)
                destroyed_sprites.insert(&sprite1);
            if (i2 == Sprite::interaction::destroyed)
                destroyed_sprites.insert(&sprite2);
        });
    };

    sprite_space.collide(sprite_callback);
    ode::collide(sprite_space, bolt_space, sprite_callback);

    world.step(tick_size);
    contact_joints.clear();

    // Only destroy after step because they may be in a contact joint, and
    // fixing the joint to the world instead of the possibly moving destroyed
    // body wouldn't be quite correct.
    for (auto i: destroyed_sprites)
        // TODO: optimize
        sprites.remove_if([&] (auto& p) { return p.get() == i; });
}


#ifndef NDEBUG
void
Island::show() const
{
    using pgamecc::operator<<;
    cout << "origin: " << origin << '\n';
    cout << "bound: " << bound << '\n';
    cout << sprites.size() << " sprites\n";
    for (auto& sprite: sprites)
        cout << "    " << sprite->body << '\n';

    cout << voxels.size() << " voxels\n";
    for (auto& v: voxels)
        cout << "    " << v.second.box << ": " << v.second.geom << '\n';
}
#endif


void
Island::render(SpriteStream& stream) const
{
    for (auto& s: sprites)
        s->render(stream);
}



//
// Sea
//

void
Sea::insert(unique_ptr<Sprite> sprite)
{
    //assert(!sprite->island);

    Box b = sprite->bound();

    if (islands.empty())
        islands.emplace_back(*this, b);
    islands.back().insert(move(sprite));
}


void
Sea::step(Grid& grid)
{
    for (auto& island: islands)
        island.sync(grid);
    for (int i = 0; i < 10; i++)
        for (auto& island: islands)
            island.tick(grid);
}


#ifndef NDEBUG
void
Sea::show() const
{
    for (auto& island: islands)
        island.show();
}
#endif


void
Sea::render(SpriteStream& stream) const
{
    for (auto& island: islands)
        island.render(stream);
}


list<const Island*>
Sea::all() const
{
    list<const Island*> result;
    for (auto& island: islands)
        result.push_back(&island);
    return result;
}
