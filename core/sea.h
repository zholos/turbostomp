#ifndef CORE_SEA_H
#define CORE_SEA_H

#include "box.h"
#include "grid.h"
#include "ode.h"

#include <list>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>

using std::enable_if_t;
using std::is_base_of;
using std::is_standard_layout;
using std::is_trivially_destructible;
using std::list;
using std::map;
using std::move;
using std::pair;
using std::unique_ptr;
using pgamecc::dvec3;

class SpriteStream;

class Island;
class Sea;


// Sprite is the base class for all dynamic objects. Objects are owned by
// islands and can migrate between them, or be dormant (not simulated but exist
// at a location, outside the influence of active objects). Object
// implementations define behaviours by overriding virtual methods and use the
// base class for physics.

// There used to be a similar Bolt class for simpler short-lived objects that
// used ode::GeomBody instead of ode::TranslatedBody, but was removed due to
// the extra complexity of two kinds of objects and not enough storage advantage
// compared to the data stored by ODE for every body.

namespace detail {

// We need to be able to back out a pointer to Sprite from a pointer to its
// ode::Body member (which gets passed through ODE). The organization involving
// base classes allows doing this without offsetof, although it's not quite
// compliant due to ode::TranslatedBody not being a standard-layout class.

struct SpriteBase {
    union { // must be first
        dloc dormant_location;    // dormant
        ode::TranslatedBody body; // active
    };
    Island* island; // null if dormant

    // If this were a standard-layout class, address of body would be guaranteed
    // to be the same as of the anonymous union and of SpriteBase itself,
    // according to [class.union] p.1 and [class.mem] p.19 (see static_assert
    // below).

    SpriteBase() : island() {}
    ~SpriteBase() { base_clear(); }

    void base_clear() {
        if (island) body.~TranslatedBody();
        island = nullptr;
    }
    void base_dormant(dloc location) {
        // TODO: unimplemented is_trivially_copyable is more accurate
        static_assert(is_trivially_destructible<dloc>::value, "");
        base_clear();
        dormant_location = location;
    }
    void base_active(Island* i, ode::TranslatedBody&& b) {
        base_clear();
        new(&body) ode::TranslatedBody(move(b));
        island = i;
    }
};

}

class Sprite : /* TODO: protected */ public detail::SpriteBase {
public:
    // lookup

    static Sprite& find(ode::Body& body) {
        //static_assert(std::is_standard_layout<detail::SpriteBase>::value, "");
        // TODO: technically this is required for the cast to be correct

        // Cast to specific kind of ode::Body, i.e. ode::TranslatedBody. Allowed
        // by [expr.static.cast] p.11 (unless we try to look for the wrong kind
        // of Derived).
        auto body_member = static_cast<ode::TranslatedBody*>(&body);

        // Cast to containing BodyMember union. The cast is equivalent to static
        // casts through void* according to [expr.reinterpret.cast] p.7. The
        // downcast to void* gives the same address according to [conv.ptr] p.2.
        // The upcast from void* gives the same address according to
        // [expr.static.cast] p.13. Thus, the cast gives a pointer to the same
        // address.
        //     Since the address of the body member and the containing union is
        // actually the same (see above), the result is a pointer to the union
        // object.
        auto body_base = reinterpret_cast<detail::SpriteBase*>(body_member);

        // Finally, cast to derived class according to [expr.static.cast] p.11.
        return *static_cast<Sprite*>(body_base);
    }


    // properties

    dloc location() const; // grid-global position and orientation

    Box bound() const;


    // behaviour

    // Override these to create geoms and joints which depend on world and thus
    // require island to be non-null. island is null before placed() is called
    // and after removing() is called.
    virtual void placed() {}
    virtual void removing() {}

    virtual void after_sync() {} // set up constraints etc.
    virtual void before_tick() {} // apply forces etc.

    enum class interaction { ignore, hit, destroyed };

    virtual interaction collide(Tile&) { return interaction::hit; }
    virtual interaction collide(Sprite&) { return interaction::hit; }


    // parameters

    virtual double bounciness() const { return 0; }
    virtual double radius() const { return 10; }


    // display

    virtual void render(SpriteStream&) {}
};



// A subset of world bodies that don't interact with any other bodies and run
// in their own separate simulation.

struct Island { // TODO: class
    Sea& sea;
    ode::World world; // constructed before all ODE objects below
    ode::JointGroup contact_joints;
    ode::Space sprite_space,
               bolt_space, // for sprites that don't collide with each other
               voxel_space;

    list<unique_ptr<Sprite>> sprites;

    // A voxel here is the physics counterpart of a grid tile.
    struct Voxel {
        ode::Geom geom; // Box or TriMesh for shape; first member
        // Only need size and shape for keeping voxels up-to-date, but keep
        // more data to simplify collision handling.
        SBox box;
        Tile tile; // kept current, no need for cursor lookup
        bool seen;
    };
    static_assert(is_standard_layout<Voxel>::value, ""); // cast &geom to Voxel
    map<ivec3, Voxel, Grid::cursor::CoordLess> voxels;

    Box bound;
    ivec3 origin; // grid-global position = world position + origin
                  // Integer to maintain precision across entire grid and to
                  // simplify casting world (local) position to integer tile
                  // coordinates.

    int next_sync;
    double tick_size = 1 / 60. / 10;

    ode::Geom create_voxel(SBox, Tile);

public:
    Island(Sea&, Box);

    void insert(unique_ptr<Sprite>);

    template<typename T, typename... Args>
    enable_if_t<is_base_of<Sprite, T>::value, T*>
    create(dloc location, Args&&... args) {
        T* sprite = new T(forward<Args>(args)...);
        unique_ptr<Sprite> owned{sprite};
        owned->dormant_location = location;
        insert(move(owned));
        return sprite;
    }

    void sync(const Grid&);

    // Simulation moves in small ticks, perhaps 600 per second.
    void tick(Grid&); // TODO: this should edit grid overlay

    // And then update voxels according to grid data.

#ifndef NDEBUG
    void show() const;
#endif

    void render(SpriteStream&) const;

    friend class Body;
};


// Collection of all islands.

class Sea {
    list<Island> islands;

public:
    void insert(unique_ptr<Sprite>);

    template<typename T, typename... Args>
    enable_if_t<is_base_of<Sprite, T>::value, T*>
    create(dloc location, Args&&... args) {
        T* sprite = new T(forward<Args>(args)...);
        unique_ptr<T> owned{sprite};
        sprite->dormant_location = location;
        insert(move(owned));
        return sprite;
    }

    // One step is one frame. The following happens:
    // - island bounds are updated
    // - islands with intersecting bounds are joined
    // - sparsely populated islands are split
    // - new islands are created for new craft
    // - island simulations tick (10 ticks per step)
    // - grid edits are incorporated into the global grid
    void step(Grid&);

#ifndef NDEBUG
    void show() const;
#endif

    void render(SpriteStream&) const;

    list<const Island*> all() const; // TODO: better access mechanism
                                     // TODO: only in frustum

    friend class Island;
};


#endif
