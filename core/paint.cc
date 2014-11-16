#include "paint.h"

#include <pgamecc.h>

using pgamecc::dvec2;
using pgamecc::ivec2;
using pgamecc::ivec4;
using pgamecc::make_image;
using pgamecc::PerlinNoise;
using pgamecc::distance2;
namespace entropy = pgamecc::entropy;


void
paint::sphere(View v, Tile t)
{
    v = v.base();
    assert(v.model_box().p0() == ivec3(0));
    ivec3 size = v.model_box().size();
    int diameter = glm::compMin(size);
    assert(size == ivec3(diameter));

    for (auto u: Box{size}.coords())
        // calculate with denominator 2: 2*center=size, 2*radius=diameter
        // use coordinates for center of unit box, i.e. u+.5
        if (distance2(size, u*2+1) < diameter*diameter)
            v[u].fill(t);
}

void
paint::sphere_smooth(View v, Tile t)
{
    v = v.base();
    assert(v.model_box().p0() == ivec3(0));
    ivec3 size = v.model_box().size();
    int diameter = glm::compMin(size);
    assert(size == ivec3(diameter));

    diameter++; // looks better

    for (auto u: Box{size}.coords()) {
        boct corners{0};
        for (auto corner: ioct::all()) {
            // see sphere() for computation
            auto v = (u + ivec3(corner.bvec3_cast()))*2;
            corners[corner] |= distance2(size, v) <= diameter*diameter;
        }
        if (corners)
            v[u].fill(t.shape(corners));
    }
}


void
paint::cylinder(View v, Tile t)
{
    v = v.base();
    assert(v.model_box().p0() == ivec3(0));
    ivec3 size = v.model_box().size();
    int diameter = glm::compMin(size.xz());
    assert(size.xz() == ivec2(diameter));

    for (auto u: Box{size}.boxes_y())
        if (distance2(size.xz(), u.p0().xz()*2+1) < diameter*diameter)
            v.clip(u).fill(t);
}


// integer mix with rational factor n/d
inline ivec3 mixr(ivec3 a, ivec3 b, ivec3 n, ivec3 d) {
    return a + (b - a) * n / d;
}

void
paint::tree(View v)
{
    // Top is a sphere, size determined by overall width.
    // Bottom is a cylinder, 1/5 in diameter.
    //   OOOO
    //  OOOOOO
    //   OOOO
    //    ||
    //    ||

    auto b = v.model_box();
    assert(b.size().x == b.size().z && b.size().x < b.size().y);
    v = v.translate(b.p1() - ivec3(0, b.size().x, 0));

    sphere_smooth(v.clip_up(), Tile{}.color({22, 16, 5}));

    auto trunk = v.rotate(irot::flip_y()).clip_up().base();
    auto t = trunk.model_box();
    auto trim = t.size() * ivec3(1, 0, 1) * 2 / 5;
    cylinder(trunk.clip(t.trim(trim, trim)), Tile{}.color({3, 19, 0}));
}


void
paint::heightmap(View v, pgamecc::Image<int> h, Tile t)
{
    v = v.base();
    assert(h.size() == v.model_box().size().xz());
    for (auto u: v.model_box().boxes_y())
        v.clip(u.p0() + Box({1, h[u.p0().xz()], 1})).fill(t);
}


void
paint::heightmap_smooth(View v, pgamecc::Image<int> h, Tile t)
{
    v = v.base();
    assert(h.size() == v.model_box().size().xz() + 1);

    for (auto u: v.model_box().boxes_y()) {
        ivec4 l;
        for (int i = 0; i < 4; i++)
            l[i] = h[u.p0().xz() + ivec2(i%2, i/2)];
        int m = glm::compMin(l);
        v.clip(u.p0() + Box{ivec3(1, m, 1)}).fill(t);
        for (int i = 0; i < 2; i++) {
            auto f = glm::greaterThan(l, glm::ivec4(m+i));
            auto top = boct::empty();
            for (int j = 0; j < 4; j++) {
                top[ioct{j%2, 0, j/2}] |= !(glm::compAdd(ivec4(f)) == 1 &&
                                            f[j^3]);
                top[ioct{j%2, 1, j/2}] |= f[j];
            }
            if (Tile top_tile = t.shape(top))
                v.clip(u.p0() + ivec3(0, m+i, 0) + SBox{1}).fill(top_tile);
        }
    }
}


static auto
rolling_hills_heightmap(ivec3 size)
{
    // TODO: random seed
    PerlinNoise noise;
    noise.set_frequency(10);

    int h_max = size.y;
    return make_image(size.xz(), [&] (auto p) {
        return glm::clamp((int)((.5*noise(p)+.5) * h_max), 0, h_max);
    });
}

void
paint::rolling_hills(View v, Tile t)
{
    heightmap(v, rolling_hills_heightmap(v.model_box().size()), t);
}

void
paint::rolling_hills_smooth(View v, Tile t)
{
    heightmap_smooth(v, rolling_hills_heightmap(v.model_box().size()+1), t);
}


void
paint::trees(View v)
{
    v = v.base();
    for (auto u: v.model_box().trim(ivec3(2, 0, 2), ivec3(2, 0, 2)).boxes_y())
        if (entropy::dice(1000) == 0) {
            int h = 0;
            v.clip(u).each_tile([&] (SBox s, Tile t) {
                if (!t.shape())
                    h = max(h, s.y1());
            });
            tree(v.clip(Box{ivec3(5, 11, 5)} + ivec3(u.x0()-2, h, u.z0()-2)));
        }
}
