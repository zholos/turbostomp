#ifndef CORE_TILE_H
#define CORE_TILE_H

#include "box.h"

#include <pgamecc.h>

#include <cassert>
#include <cstdint>

#include <glm/glm.hpp>

using pgamecc::dquat;
using pgamecc::dloc;
using pgamecc::boct;


// Tile, Node and Branch are companion classes. They are declared in this
// sequence because notionally a Tile is contained in a Node and a Branch
// contains several Nodes. Tile and Node share the same data, but the former is
// copyable and used for mutating the data, and the latter is a non-copyable
// part of the grid and used to handle the type tag.

// A simple tile that can be represented in a fixed number of bits. The data
// member is the same as in Node, including the tag in bit 0. Immutable when
// assigned to a cell. To change, copy, modify, and assign back.

class Tile {
    friend class Node;
    typedef uintptr_t data_type; // later assumed unsigned for bit shifts
    data_type data;

    explicit Tile(data_type data) : data(data) {
        assert(data & 1);
    }

    bool bit(int from) const {
        assert(from >= 1);
        return data & 1 << from;
    }

    int bits(int from, int count) const {
        assert(from >= 1 && count > 0);
        // TODO: check maximum number of bits
        return data >> from & (1 << count) - 1;
    }

    Tile set_bits(int from, int count, int value) const {
        data_type mask = (static_cast<data_type>(1) << count) - 1 << from;
        data_type set = static_cast<data_type>(value) << from;
        assert(!(set & ~mask));
        return Tile{data & ~mask | set};
    }

public:
    Tile() : data(3) {
        assert(*this); // not empty
    }

    // Representation:
    //   bit 0: 1 - simple tile tag as in Node
    //   bits 1-3: hit points (3 bits)
    //     - 0 - empty
    //     - 1 - normal, one hit to destroy
    //     - max - invulnerable
    //   bits 4-18: color (15 bits)
    //     - red (5 bits),
    //     - green (5 bits),
    //     - blue (5 bits)
    //   bits 19-21: alpha (3 bits, logarithmic)
    //   bits 22-26: shape (5 bits)
    //     - 0 - cube
    //     - 1-12 - ramp: rotate_x(0-3)+4*rotate_xyz(0-2)
    //     - 13-20 - corner1: which simplex is filled
    //     - 21-28 - corner2: which simplex is cut out
    enum {
        tag_bit = 0,
        hp_bits = 1, hp_size = 3,
        color_bits = 4, color_size = 15,
        shape_bits = 22, shape_size = 5,
        end_bit = 27
    };
    static_assert(end_bit <= sizeof(data_type) * 8, "");

    static Tile empty() { return Tile(1); };

    operator bool() const {
        // TODO: check whether this is just a "test" instruction in assembly
        return bits(hp_bits, hp_size);
    }

    Tile hp(int v) {
        assert(0 <= v && v < (1 << hp_size) - 1);
        return set_bits(hp_bits, hp_size, v);
    }
    Tile invulnerable() {
        return set_bits(hp_bits, hp_size, (1 << hp_size) - 1);
    }
    Tile hit(int damage = 1) {
        int hp = bits(hp_bits, hp_size);
        if (hp && hp != (1 << hp_size) - 1)
            hp--;
        return set_bits(hp_bits, hp_size, hp);
    }

    ivec3 color() const { return { bits(color_bits,    5),
                                   bits(color_bits+5,  5),
                                   bits(color_bits+10, 5) }; }

    glm::vec4 color_vec4() const {
        auto c = pgamecc::color::sRGB{bits(color_bits,    5) / 31.,
                                      bits(color_bits+5,  5) / 31.,
                                      bits(color_bits+10, 5) / 31.}.rgb();
        return glm::vec4(c.r, c.g, c.b, 1);
    }

    Tile color(ivec3 c) const { // 0-31, sRGB
        assert(c.r >= 0 && c.r < 32 &&
               c.g >= 0 && c.g < 32 &&
               c.b >= 0 && c.b < 32);
        return set_bits(color_bits, color_size, c.r | c.g << 5 | c.b << 10);
    }

    // short so it can be packed with stuff
    short shape() const { return bits(shape_bits, shape_size); }
    short shape_mesh() const;
    dquat shape_quat() const;
    dloc shape_loc() const; // assumes size 1
    Tile shape(boct corners) const;

    bool operator==(Tile r) const { return data == r.data; }
    bool operator!=(Tile r) const { return !((*this) == r); }

#ifndef NDEBUG
    friend ostream& operator<<(ostream&, Tile);
#endif
};


#endif
