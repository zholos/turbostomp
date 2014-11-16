#ifndef CORE_OCTANT_H
#define CORE_OCTANT_H

#include "box.h"

#include <pgamecc.h>

using pgamecc::ivec3;
using pgamecc::dvec3;
using pgamecc::dvec4;


// values picked for bitwise operations
enum class ConvexTest { outside = 0, partial = 1, inside = 3 };

inline ConvexTest operator&(ConvexTest a, ConvexTest b) {
    return static_cast<ConvexTest>(static_cast<int>(a) & static_cast<int>(b));
};


template<int N>
struct Convex {
    const dvec4 planes[N];

    bool sphere_outside(dvec3 center, double radius) const {
        for (int i = 0; i < N; i++)
            if (glm::dot(planes[i], dvec4(center, 1)) < -radius)
                return true;
        return false;
    }

    bool box_outside(Box b) const {
        for (int i = 0; i < N; i++) {
            auto p = glm::mix(dvec3(b.p0()),
                              dvec3(b.p1()),
                              glm::greaterThan(dvec3(planes[i]), dvec3()));
            if (glm::dot(planes[i], dvec4(p, 1)) < 0)
                return true;
        }
        return false;
    }

    ConvexTest box_test(SBox b) const {
        bool all_inside = true;
        for (int i = 0; i < N; i++) {
            auto p = glm::mix(dvec3(b.p0()),
                              dvec3(b.p1()),
                              glm::greaterThan(dvec3(planes[i]), dvec3()));
            double depth = glm::dot(planes[i], dvec4(p, 1));
            if (depth < 0)
                return ConvexTest::outside;
            else
                // Project plane normal onto box diagonal. Instead of
                // constructing the diagonal, the normal is adjusted to match a
                // diagonal of (1, 1, 1), hence compAdd instead of dot().
                if (depth < b.size() * glm::compAdd(glm::abs(dvec3(planes[i]))))
                    all_inside = false;
        }
        return all_inside ? ConvexTest::inside : ConvexTest::partial;
    }
};


struct Octant {
    const ivec3 origin;
    const ioct octant;

    bool point_inside(ivec3 p) const {
        return ioct{glm::greaterThanEqual(p, origin)} == octant;
    }

    ConvexTest box_test(SBox b) const {
        // h and l are the high and low bits of the three ConvexTest values
        // testing the box against each of three axes

        // test for octant 0
        int h = ioct{glm::lessThan(b.p0(), origin)}.i(),
            l = ioct{glm::lessThan(b.p1(), origin)}.i();

        // reverse each test for the correct octant
        h ^= octant.i(); l ^= octant.i(); // flip bits (1 becomes 2)
        int h_ = h; h &= l; l |= h_; // normalize 2 to 1

        // combine tests for final result
        return static_cast<ConvexTest>((h == 7) << 1 | l == 7);
    }
};


#endif
