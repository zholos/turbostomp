#ifndef CORE_GRID_H
#define CORE_GRID_H

#include "box.h"
#include "tile.h"

#include <pgamecc.h>

#include <cassert>
#include <functional>
#include <list>
#include <memory>
#include <stack>
#include <type_traits>
#include <utility>

#include <boost/noncopyable.hpp>

#include <glm/glm.hpp>

using std::alignment_of;
using std::exchange;
using std::function;
using std::list;
using std::pair;
using std::stack;
using std::unique_ptr;
using boost::noncopyable;
using pgamecc::ivec3;
using pgamecc::iloc;
using pgamecc::dloc;


// A pointer to an octree node, with type information embedded in the pointer
// rather than what is pointed to.

class Branch;

// TODO: check whether this is moveable, and required to be moveable
class Node : noncopyable {
    typedef Tile::data_type data_type;
    data_type data;

    // Representation:
    //   bit 0: 0 - null or branch
    //          1 - simple tile
    // Chosen this way so branch pointer masking is not required.
    // Simple tiles need to be masked anyway to get different parts.
    // In the future there may be another kind of pointer for complex tiles.
    // Branches are pointers and owned by us.
    // Empty is a kind of tile. Null means unknown, to be used in future for
    // overlays.

    // Can't test this statically, but assume that nullptr has bit 0 clear due
    // to allignment restriction and so never interferes with tile data space.
    //static_assert(
    //    reinterpret_cast<data_type>(static_cast<void*>(nullptr)) == 0,
    //    "NULL is not 0");

    // also see check for alignment_of below

    Node(data_type data) : data(data) {}

    bool is_pointer() const {
        return !(data & 1);
    }

    void set_pointer(void* pointer) {
        data = reinterpret_cast<data_type>(pointer);
        assert(is_pointer());
    }

    void* get_pointer() const {
        assert(is_pointer());
        return reinterpret_cast<void*>(data);
    }

    data_type release() { return exchange(data, 0); }

    void clear(); // defined below

public:
    Node() {
        set_pointer(nullptr);
        assert(is_null());
    }

    Node(Node&& r) : data(r.release()) {}

    Node& operator=(Node&& r) {
        clear();
        data = r.release();
        return *this;
    }

    Node(const Tile& t) : data(t.data) {
        assert(is_tile());
    }

    Node& operator=(Tile t) {
        clear();
        data = t.data;
        assert(is_tile());
        return *this;
    }

    Node(unique_ptr<Branch> b) {
        set_pointer(b.release());
        assert(is_branch());
    }

    Node& operator=(unique_ptr<Branch> b) {
        clear();
        set_pointer(b.release());
        assert(is_branch());
        return *this;
    }

    ~Node() {
        clear();
    }

    bool is_null() const {
        return is_pointer() && !get_pointer();
    }

    bool is_branch() const {
        return is_pointer() && get_pointer();
    }

    bool is_tile() const {
        return !is_pointer();
    }

    Branch& branch() const {
        assert(is_branch());
        return *static_cast<Branch*>(get_pointer());
    }

    Tile tile() const {
        assert(is_tile());
        return Tile{data};
    }

#ifndef NDEBUG
    friend ostream& operator<<(ostream&, const Node&);
#endif
};


// A node in the octree.

class Branch {
    Node child[8];

public:
    explicit Branch() : child{} {}
    explicit Branch(Tile t) : child{t, t, t, t, t, t, t, t} {}

    Node& operator[](ioct i) {
        assert(i.i() >= 0 && i.i() < 8);
        return child[i.i()];
    }
};


// for Node
static_assert(
    alignment_of<Branch>::value >= 2, // 1 bit
    "not enough bits for tag");

inline void
Node::clear()
{
    if (is_branch())
        delete &branch();
    // now overwrite data
}



// Cursor is used to access and transform the octree. It is a wrapper for a Node
// pointer. It is not meant to persist beyond one operation.

namespace detail {

class CursorCommon {
protected:
    const SBox s;

    CursorCommon(SBox s) : s(s) {}

public:
    SBox box() const { return s; }

    static SBox containing_sbox(SBox);

    static bool coord_less(ivec3, ivec3);
    struct CoordLess {
        bool operator()(ivec3 a, ivec3 b) const {
            return CursorCommon::coord_less(a, b);
        }
    };
};

template<typename Derived, typename Node>
class CursorBase_ : public CursorCommon {
private:
    Derived copy() const { return { node, s }; }

protected:
    typedef CursorBase_ CursorBase;

    Node& node;

public:
    CursorBase_(Node& node, SBox s) : CursorCommon(s), node(node) {}

    Derived operator[](ioct i) const {
        assert(is_branch());
        return { node.branch()[i], s.leaf(i) };
    }

    Derived find(SBox b) const {
        if (b.size() < s.size())
            return (*this)[glm::greaterThanEqual(b.p0(), s.center())].find(b);
        assert(b == s);
        return copy();
    }

    Derived find_smallest(SBox b) const {
        assert(s.contains(b));
        if (b.size() < s.size() && is_branch())
            return (*this)[
                glm::greaterThanEqual(b.p0(), s.center())].find_smallest(b);
        return copy();
    }

    bool is_null() const { return node.is_null(); }
    bool is_branch() const { return node.is_branch(); }
    bool is_tile() const { return node.is_tile(); }
    Tile tile() const { return node.tile(); }
};

class ConstCursor : public CursorBase_<ConstCursor, const Node> {
    using CursorBase::CursorBase;

public:
    // Iterate over tiles. Used by Island to map tiles to ODE boxes. Order is
    // fixed as defined by coord_less.
    void each_tile(Box bound, function<void(SBox, Tile)>) const;

#ifndef NDEBUG
    void show_text(int indent = 0) const;
#endif
};

class Cursor : public CursorBase_<Cursor, Node> {
    using CursorBase::CursorBase;

    void subdivide() const {
        if (!is_branch())
            node = unique_ptr<Branch>(
                is_null() ? new Branch() : new Branch(tile()));
    }

    bool fill_recurse(Box b, Tile t) const;

public:
    void cut(Box b) const { fill_recurse(b, Tile::empty()); }
    void fill(Box b, Tile t) const { fill_recurse(b, t); }
};

}


// The size of each node is calculated dynamically by the cursor from the value
// stored for the root node here.

class Grid {
    Node root;
    int _size;

public:
    using       cursor = detail::Cursor;
    using const_cursor = detail::ConstCursor;

    Grid() : _size(1) {} // placeholder grid
    Grid(int size) : _size(size) { assert(size > 0); }
    int size() const { return _size; }
          cursor top()       { return { root, SBox{_size} }; }
    const_cursor top() const { return { root, SBox{_size} }; }
    const_cursor ctop() const { return top(); }
};


// View is a window into a clipped box of grid space, rotated (in 90-degree
// increments) and translated into a local (model) space. This helps to build
// similar tile models at different locations in the grid.

// The initial view matches grid coordinates, with the origin at the base (lower
// corner) of the grid.

class View {
    Grid* grid; // pointer so View can be assigned to
    Box b; // model coordinates
    iloc l; // model to grid transform

    View(Grid* grid, Box b, iloc l) :
        grid(grid), b(b), l(l) {}

    View set_model_box(Box m) const {
        assert(b.contains(m));
        return { grid, m, l };
    }

    View set_location(iloc r) const {
        View v(grid, ~r * l * b, r);
        assert(v.grid_box() == grid_box());
        return v;
    }

public:
    View(Grid& grid) :
        grid(&grid),
        b(SBox{grid.size()})
    {
    }

    Box model_box() const { return b; }
    Box grid_box() const { return l * b; }


    // transformations (grid box doesn't change but model box coordinates do)

    View translate(ivec3 new_origin) const {
        return set_location(l * iloc{new_origin, {}});
    }

    View base() const {
        View v = translate(model_box().p0());
        assert(v.model_box().p0() == ivec3(0));
        return v;
    }

    View center() const {
        View v = translate(model_box().center());
        assert(v.model_box().center() == ivec3(0));
        return v;
    }

    View rotate(irot r) const {
        return set_location(l * iloc{{}, ~r});
    }


    // clipping (grid box is reduced in size)

    // removes below infinite x-z plane
    View clip_up() const {
        auto p0 = model_box().p0();
        p0.y = max(p0.y, 0);
        return set_model_box(Box::ranged(p0, model_box().p1()));
    }

    View clip(Box r) const {
        return set_model_box(model_box().intersection(r));
    }

    View operator[](ivec3 r) const {
        return clip(r + SBox{1});
    }


    // operations (edit tiles in grid box)

    void cut() const {
        grid->top().cut(grid_box());
    }

    void fill(Tile t) const {
        grid->top().fill(grid_box(), t);
    }


    // queries

    // model to grid
    dloc location(dloc p) { return l.dloc_cast() * p; }

    void each_tile(function<void(SBox, Tile)>) const;

#ifndef NDEBUG
    void show_oblique() const;
#endif
};


#endif
