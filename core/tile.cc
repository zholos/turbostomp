#include "tile.h"

#include <string>

using std::string;


// Ramps have 6 vertices set and are symmetrical along a principal axis. For
// each axis there are 4 rotations, identified by which corner in the plane
// perpendicular to the axis is not set (and thus the normal vector of the
// largest face).
//
//          y^
//           |
//           *------*                     *--*    *  o    *--*    o  *
//          /      /|                     | /     |\       \ |      /|
//         /      / |                     |/      | \       \|     / |
//   <--  *------*--* ---->               *  o    *--*    o  *    *--*
//    z                  x
//          x-axis           rotation:     0       1       2       3
//                             normal:   -1,-1    1,-1    1,1    -1,1
//                     model rotation:    180     270     90       0
//

namespace {
struct ShapeLookup {
    signed char corners_to_shape[256];
    enum { shapes = 29 };
    int shape_to_mesh[shapes];
    dquat shape_to_quat[shapes];

    ShapeLookup() {
        for (auto& c: corners_to_shape)
            c = -1;

        int s = 0;
        auto emit = [&] (int mesh, boct model, irot r) {
            corners_to_shape[(r * model).i()] = s;
            shape_to_mesh[s] = mesh;
            shape_to_quat[s] = r.quat_cast();
            s++;
        };

        emit(0, ~boct::empty(), {});

        for (int xyz = 0; xyz < 3; xyz++)
            for (int x = 0; x < 4; x++)
                emit(1, boct{0x3f}, irot::rotate_xyz(xyz) * irot::rotate_x(x));

        for (auto i: ioct::all())
            emit(2, boct{0x17}, irot::face(i));
        for (auto i: ioct::all())
            emit(3, boct{0x7f}, irot::face(i));

        assert(s == shapes);
    }
} shape_lookup;
}

short
Tile::shape_mesh() const
{
    int s = shape();
    assert(0 <= s && s < shape_lookup.shapes);
    return shape_lookup.shape_to_mesh[s];
}

dquat
Tile::shape_quat() const
{
    int s = shape();
    assert(0 <= s && s < shape_lookup.shapes);
    return shape_lookup.shape_to_quat[s];
}

dloc
Tile::shape_loc() const
{
    auto q = shape_quat();
    return dloc{dvec3{.5}, q} * dloc{dvec3(-.5), {}};
}

Tile
Tile::shape(boct corners) const
{
    int s = shape_lookup.corners_to_shape[corners.i()];
    if (s < 0)
        return Tile::empty();
    else
        return set_bits(shape_bits, shape_size, s);
}


#ifndef NDEBUG
ostream&
operator<<(ostream& s, Tile t) {
    if (!t)
        return s << "Empty";

    auto c = t.color_vec4();
    auto f = [](double v) {
        char c[3];
        std::sprintf(c, "%02x", (int)(v * 256));
        return string(c);
    };

    return s << "Tile{#" << f(c.x) << f(c.y) << f(c.z) << "}";
}
#endif
