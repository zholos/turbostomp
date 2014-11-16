#include "box.h"

#include <iostream>

using std::cout;
using std::list;


SBox
Box::sbox() const {
    assert(d.x == d.y && d.y == d.z);
    return p + SBox{d.x};
}


#ifndef NDEBUG

bool
in_boxes(const list<Box>& boxes, ivec3 v)
{
    for (Box b: boxes)
        if (b.contains(Box{ivec3(1)} + v))
            return true;
    return false;
}

void
Box::show_2d_cells(const list<Box>& boxes, int extent) {
    for (int y = extent-1; y >= -extent; y--) {
        for (int x = -extent; x < extent; x++) {
            if (x == 0)
                cout << '|';
            if (in_boxes(boxes, ivec3(x, y, 0)))
                cout << "[]";
            else
                cout << "  ";
        }
        cout << '\n';
        if (y == 0) {
            for (int x = -extent; x < extent; x++) {
                if (x == 0)
                    cout << '|';
                cout << "--";
            }
            cout << '\n';
        }
    }
}


void
Box::show_2d(const list<Box>& boxes, int extent) {
    auto in = [&](int x, int y) { return in_boxes(boxes, ivec3(x, y, 0)); };

    for (int y = extent-1; y >= -extent; y--) {
        for (int x = -extent; x < extent; x++) {
            if (!in(x, y))
                cout << "  ";
            else {
                for (int i: { -1, 1 }) {
                    if (in(x, y-1) && in(x, y+1))
                        if (!in(x+i, y))
                            cout << '|';
                        else if (in(x+i, y) && in(x+i, y-1) && in(x+i, y+1))
                            cout << ' ';
                        else
                            cout << '+';
                    else if (!in(x, y-1) && !in(x, y+1) && in(x+i, y))
                        cout << '=';
                    else if (in(x, y-1) && in(x+i, y-1) && in(x+i, y))
                        cout << '-';
                    else if (in(x, y+1) && in(x+i, y+1) && in(x+i, y))
                        cout << '-';
                    else
                        cout << '+';
                }
            }
        }
        cout << '\n';
    }
}


void
Box::show_oblique(const list<Box>& boxes, int extent, bool axes) {
    auto in = [&](int x, int y, int z) {
        return in_boxes(boxes, ivec3(x, y, z));
    };

    auto face = [&](int a, int x, int y, int z) {
        return in(x, y, z) != in(x-(a==1), y-(a==2), z-(a==3));
    };

    auto edge = [&](int a, int x, int y, int z) {
        int b = a % 3 + 1, c = b % 3 + 1;
        return face(b, x, y, z) != face(b, x-(c==1), y-(c==2), z-(c==3)) ||
               face(c, x, y, z) != face(c, x-(b==1), y-(b==2), z-(b==3));
    };

    auto vertex = [&](int x, int y, int z) {
        return (edge(1, x, y, z) || edge(1, x-1, y, z)) +
               (edge(2, x, y, z) || edge(2, x, y-1, z)) +
               (edge(3, x, y, z) || edge(3, x, y, z-1)) > 1;
    };

    // draw on edges, not on unit boxes
    for (int sy = extent; sy >= -extent; sy--) {
        for (int sx = -extent*2; sx <= extent*2; sx++) {
            char c = 0;
            for (int z = -extent; z <= extent; z++) {
                int x = sx + z,
                    y = sy + z;
                int half = !!(x % 2);
                x = (x-half) / 2;

                if (!half && axes)
                    if (x == 0 && y == 0 && z == 0)
                        c = '0';
                    else if (y == 0 && z == 0)
                        c = '.';
                    else if (x == 0 && z == 0)
                        c = '\'';
                    else if (x == 0 && y == 0)
                        c = ',';

                if (!half && face(1, x, y, z) ||
                        face(2, x, y, z) || face(3, x, y, z))
                    c = ' ';

                if (half) {
                    if (edge(1, x, y, z))
                        c = '-';
                } else {
                    if (vertex(x, y, z))
                        c = '+';
                    else if (edge(1, x, y, z))
                        c = '-';
                    else if (edge(2, x, y, z))
                        c = '|';
                    else if (edge(3, x, y, z))
                        c = '/';
                }
            }
            cout << (c ? c : ' ');
        }
        cout << '\n';
    }
}

#endif
