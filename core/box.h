#ifndef CORE_BOX_H
#define CORE_BOX_H

#include <pgamecc.h>

#include <algorithm>
#include <cstdlib> // abs
#include <iostream>
#include <list>

#include <glm/ext.hpp>

using std::ostream;
using std::max;
using std::min;
using std::abs;
using std::swap;
using pgamecc::ivec3;
using pgamecc::dvec3;
using pgamecc::irot;
using pgamecc::iloc;
using pgamecc::ioct;
using pgamecc::mod_down;


// Axis-aligned box with integer coordinates

class SBox;

class Box {
    ivec3 p;
    ivec3 d;

    Box(ivec3 p, ivec3 d) : p(p), d(d) {}

public:
    explicit Box(ivec3 d) : p(), d(d) {
        // zero-size boxes sometimes created as intersection
        assert(glm::compMin(d) >= 0);
    }

    static Box ranged(ivec3 p0, ivec3 p1) {
        return { p0, p1 - p0 };
    }

    // like ranged() but can be any two corners
    static Box spanning(ivec3 p0, ivec3 p1) {
        if (p0.x > p1.x) swap(p0.x, p1.x);
        if (p0.y > p1.y) swap(p0.y, p1.y);
        if (p0.z > p1.z) swap(p0.z, p1.z);
        return Box::ranged(p0, p1);
    }


    // properties

    ivec3 p0() const { return p; }
    int x0() const { return p.x; }
    int y0() const { return p.y; }
    int z0() const { return p.z; }

    ivec3 p1() const { return p + d; }
    int x1() const { return p.x + d.x; }
    int y1() const { return p.y + d.y; }
    int z1() const { return p.z + d.z; }

    ivec3 size() const { return d; }

    bool empty() const { return !(d.x && d.y && d.z); }

    SBox sbox() const;


    // operators

    Box operator+(ivec3 v) const { return { p + v, d }; }
    Box operator-(ivec3 v) const { return { p - v, d }; }
    friend Box operator+(ivec3 v, Box b) { return b + v; }

    bool operator==(Box r) { return p == r.p && d == r.d; }
    bool operator!=(Box r) { return !operator==(r); }

    friend Box operator*(irot r, Box b) {
        return Box::spanning(r * b.p0(), r * b.p1());
    }

    friend Box operator*(iloc l, Box b) {
        return Box::spanning(l * b.p0(), l * b.p1());
    }


    // special

    auto center() const { return p + d/2; }
    auto center_dvec3() const { return dvec3(p) + .5*dvec3(d); }


    // combinations

    bool
    intersects(Box b) const {
        return x1() > b.x0() && x0() < b.x1() &&
               y1() > b.y0() && y0() < b.y1() &&
               z1() > b.z0() && z0() < b.z1();
    }

    bool
    contains(Box b) const {
        return x0() <= b.x0() && x1() >= b.x1() &&
               y0() <= b.y0() && y1() >= b.y1() &&
               z0() <= b.z0() && z1() >= b.z1();
    }

    Box
    intersection(Box b) const {
        ivec3 v0 = glm::max(p0(), b.p0());
        ivec3 v1 = glm::min(p1(), b.p1());
        return ranged(v0, glm::max(v0, v1));
    }

    Box
    combination(Box b) const {
        ivec3 v0 = glm::min(p0(), b.p0());
        ivec3 v1 = glm::max(p1(), b.p1());
        return ranged(v0, v1);
    }

    Box operator&(Box b) const { return intersection(b); }
    Box operator|(Box b) const { return combination(b); }
    Box& operator&=(Box b) { return *this = intersection(b); }
    Box& operator|=(Box b) { return *this = combination(b); }

    Box
    trim(ivec3 t0, ivec3 t1) const {
        return Box::ranged(p0() + t0, p1() - t1);
    }


    // coordinate iterators

    template<typename Coords>
    struct CoordsBase {
        // can't use Box here and can't use a reference because then
        // Box{}.coords() creates a dangling reference, so just copy the data
        const ivec3 p, d;
        CoordsBase(const Box& b) : p(b.p), d(b.d) {}

        const Coords& self() const { return static_cast<const Coords&>(*this); }

        class iterator {
            const Coords& p;
            ivec3 c;

        public:
            iterator(const Coords& p, ivec3 c) : p(p), c(c) {}

            bool operator==(iterator r) const { return c == r.c; }
            bool operator!=(iterator r) const { return !(*this == r); }

            auto operator*() const -> decltype(p.item(c)) { return p.item(c); }

            iterator& operator++() {
                c = p.next_coord(c);
                return *this;
            }
        };

        iterator begin() const {
            return { self(), self().begin_coord() };
        }
        iterator end() const {
            return { self(), self().end_coord() };
        }
    };

    struct Coords : CoordsBase<Coords> {
        // for empty boxes ensure begin_coord() == end_coord()
        Coords(Box b) : CoordsBase(b.empty() ? Box{b.p, ivec3()} : b) {}

        ivec3 next_coord(ivec3 c) const {
            if (++c.x == d.x) {
                c.x = 0;
                if (++c.y == d.y) {
                    c.y = 0;
                    ++c.z;
                }
            }
            return c;
        }

        ivec3 begin_coord() const { return {}; }
        ivec3 end_coord() const { return ivec3(0, 0, d.z); }
        ivec3 item(ivec3 c) const { return p + c; }
    };

    // iterate over each coordinate in box
    Coords coords() const { return *this; }

    struct Boxes : CoordsBase<Boxes> {
        const ivec3 base, step;

        Boxes(Box b, Box ref) :
            CoordsBase(b),
            base(mod_down(ref.p - b.p + (ref.d-1), ref.d) - (ref.d-1)),
            step(ref.d)
        {
            assert(glm::compMin(step) > 0);
        }

        ivec3 next_coord(ivec3 c) const {
            c.x += step.x;
            if (c.x >= d.x) {
                c.x = base.x;
                c.y += step.y;
                if (c.y >= d.y) {
                    c.y = base.y;
                    c.z += step.z;
                    if (c.z >= d.z)
                        c = d; // simpler than calculating real end point
                }
            }
            return c;
        }

        ivec3 begin_coord() const { return base; }
        ivec3 end_coord() const { return d; }

        Box item(ivec3 c) const {
            return p + Box{d}.intersection(c + Box{step});
        }
    };

    // iterate over each sub-box of step.size(), aligned to step.p0()
    Boxes boxes(Box step) const { return { *this, step }; }
    Boxes boxes_y() const {
        return boxes(p0() + Box{ivec3(1, d.y ? d.y : 1, 1)});
    }


#ifndef NDEBUG
    friend ostream& operator<<(ostream& s, const Box b) {
        return s << "Box{" << b.x0() << ":" << b.x1() << ", "
                           << b.y0() << ":" << b.y1() << ", "
                           << b.z0() << ":" << b.z1() << "}";
    }

    static void show_2d_cells(const std::list<Box>&, int extent = 10);
    static void show_2d(const std::list<Box>&, int extent = 10);
    static void show_oblique(const std::list<Box>&,
                             int extent = 10, bool axes = true);
#endif
};


// A cubic box. Interface like Box, but not using templates because that just
// makes things too complicated.

class SBox {
    ivec3 p;
    int d;

    SBox(ivec3 p, int d) : p(p), d(d) {}

public:
    explicit SBox(int d) : p(), d(d) {
        assert(d >= 0);
    }

    // properties

    ivec3 p0() const { return p; }
    int x0() const { return p.x; }
    int y0() const { return p.y; }
    int z0() const { return p.z; }

    ivec3 p1() const { return p + d; }
    int x1() const { return p.x + d; }
    int y1() const { return p.y + d; }
    int z1() const { return p.z + d; }

    int size() const { return d; }

    operator Box() const { return p + Box{ivec3(d)}; }

    // combinations

    bool intersects(Box b) const { return Box{*this}.intersects(b); }
    bool contains(Box b) const { return Box{*this}.contains(b); }

    // operators

    SBox operator+(ivec3 v) const { return { p + v, d }; }
    SBox operator-(ivec3 v) const { return { p - v, d }; }
    friend SBox operator+(ivec3 v, SBox b) { return b + v; }

    bool operator==(SBox r) { return p == r.p && d == r.d; }
    bool operator!=(SBox r) { return !operator==(r); }

    // special

    SBox leaf(ioct i) const { return { p + i * (d/2), d/2 }; }
    auto center() const { return p + d/2; }
    auto center_dvec3() const { return dvec3(p) + .5*d; }

    glm::mat4 mat4_cast() const {
        return glm::mat4(d, 0, 0, 0,
                         0, d, 0, 0,
                         0, 0, d, 0,
                         p.x, p.y, p.z, 1);
    }

#ifndef NDEBUG
    friend ostream& operator<<(ostream& s, const SBox b) {
        return s << "SBox{" << b.x0() << ", "
                            << b.y0() << ", "
                            << b.z0() << " : "
                            << b.size() << "}";
    }
#endif
};

#endif
