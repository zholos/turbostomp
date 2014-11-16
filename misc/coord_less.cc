// coord_less is a comparator that corresponds to the order in which the grid is
// recursively traversed. This file checks the calculation correctness.

#include <cassert>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::ivec3;


const int size = 8;


int
BVec_i(bool x, bool y, bool z)
{
    return x | y << 1 | z << 2;
}


bool
comp(ivec3 a, ivec3 b)
{
    assert(glm::compMin(a) >= 0 && glm::compMin(b) >= 0);
    int i = 0, j = 0;
    for (int d = 1;; d <<= 1) {
        if (a == b)
            return i < j;
        i = BVec_i(a.x & d, a.y & d, a.z & d);
        j = BVec_i(b.x & d, b.y & d, b.z & d);
        a = ivec3(a.x & ~d, a.y & ~d, a.z & ~d),
        b = ivec3(b.x & ~d, b.y & ~d, b.z & ~d);
    }
}


bool
equiv(ivec3 a, ivec3 b)
{
    return !comp(a, b) && !comp(b, a);
}


void
check_compare_concept()
{
    ivec3 a, b, c;

    for (a.x = 0; a.x < size; a.x++)
    for (a.y = 0; a.y < size; a.y++)
    for (a.z = 0; a.z < size; a.z++) {
        assert(!comp(a, a));
        assert(equiv(a, a));

        for (b.x = 0; b.x < size; b.x++)
        for (b.y = 0; b.y < size; b.y++)
        for (b.z = 0; b.z < size; b.z++) {
            if (comp(a, b))
                assert(!comp(b, a));
            if (equiv(a, b))
                assert(equiv(b, a));

            for (c.x = 0; c.x < size; c.x++)
            for (c.y = 0; c.y < size; c.y++)
            for (c.z = 0; c.z < size; c.z++) {
                if (comp(a, b) && comp(b, c))
                    assert(comp(a, c));
                if (equiv(a, b) && equiv(b, c))
                    assert(equiv(a, c));
            }
        }
    }
}


void
check_traversal()
{
    struct Traverse {
        ivec3 last = ivec3(-1);
        void traverse(ivec3 p, int d) {
            if (last.x >= 0)
                assert(last == p || comp(last, p));
            last = p;

            if (d > 1) {
                ivec3 s;
                int h = d/2;
                // order of iteration is important here
                for (s.z = 0; s.z < d; s.z += h)
                for (s.y = 0; s.y < d; s.y += h)
                for (s.x = 0; s.x < d; s.x += h)
                    traverse(p + s, d/2);
            }
        }
    };
    Traverse().traverse(ivec3(0), size);
}


int
main()
{
    check_compare_concept();
    check_traversal();
}
