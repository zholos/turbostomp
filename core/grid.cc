#include "grid.h"

#include "camera.h"
#include "debug.h"

#include <algorithm>
#include <iostream>

using std::cout;
using std::max;


//
// Node
//

#ifndef NDEBUG
ostream&
operator<<(ostream& s, const Node& n) {
    if (n.is_null())
        s << "Null";
    else if (n.is_tile())
        s << n.tile();
    else if (n.is_branch())
        s << "Branch";
    else
        assert(false);
    return s;
}
#endif



//
// Grid and cursor
//

SBox
detail::CursorCommon::containing_sbox(SBox b)
{
    int d = b.size() * 2;
    int mask = ~(d-1);
    return ivec3(b.x0() & mask, b.y0() & mask, b.z0() & mask) + SBox(d);
}

bool
detail::CursorCommon::coord_less(ivec3 a, ivec3 b)
{
    assert(glm::compMin(a) >= 0 && glm::compMin(b) >= 0);
    int i = 0, j = 0;
    for (int d = 1;; d <<= 1) {
        if (a == b)
            return i < j;
        i = ioct(a.x & d, a.y & d, a.z & d).i();
        j = ioct(b.x & d, b.y & d, b.z & d).i();
        a = ivec3(a.x & ~d, a.y & ~d, a.z & ~d),
        b = ivec3(b.x & ~d, b.y & ~d, b.z & ~d);
    }
}


void
detail::ConstCursor::each_tile(Box bound,
                               function<void(SBox, Tile)> callback) const
{
    // NOTE: client code may expect this to iterate over complete tiles;
    // investigate before changing this to subdivide by bound
    if (bound.intersects(s))
        if (!is_tile())
            for (auto i: ioct::all())
                (*this)[i].each_tile(bound, callback);
        else if (Tile t = tile())
            callback(s, t);
}

#ifndef NDEBUG
void
detail::ConstCursor::show_text(int indent) const
{
    for (int i = 0; i < indent; i++)
        cout << "  ";
    cout << node << " at " << s << '\n';
    if (is_branch())
        for (auto i: ioct::all())
            (*this)[i].show_text(indent + 1);
}
#endif


// TODO: this should be made faster
bool
detail::Cursor::fill_recurse(Box b, Tile t) const
{
    if (b.contains(s))
        node = t;
    else if (b.intersects(s)) {
        // NOTE: it's assumed that complex tiles are only created with size 1,
        // so we don't check to prevent subdividing complex tiles
        subdivide();
        bool all_same = true;
        for (auto i: ioct::all())
            all_same &= (*this)[i].fill_recurse(b, t);
        if (all_same && (!t || !t.shape()))
            node = t;
    }
    return node.is_tile() && node.tile() == t;
}



//
// View
//

void
View::each_tile(function<void(SBox, Tile)> callback) const
{
    grid->ctop().each_tile(
        grid_box(),
        [&] (SBox s, Tile t) { callback((~l * Box{s}).sbox(), t); });
}


#ifndef NDEBUG
void
View::show_oblique() const
{
    list<Box> r;
    grid->ctop().each_tile(
        grid_box(),
        [&] (SBox s, Tile) { r.push_back(~l * (Box{s} & grid_box())); });
    auto b = model_box();
    auto extent = max(glm::compMax(-b.p0()), glm::compMax(b.p1()));
    Box::show_oblique(r, extent * 2);
}
#endif
