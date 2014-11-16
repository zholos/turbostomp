#include "box.h"

#include <iostream>
#include <list>

using std::cout;
using std::list;

using namespace pgamecc;


int
main()
{
    ivec3 v(1, 2, 3);

    cout << v << '\n';
    cout << v + 5 << '\n';

    Box b{ivec3(10, 15, 20)};

    cout << b << '\n';
    cout << b + v << '\n';

    cout << irot::rotate_x(1) * v << '\n';

    cout << '\n';

    list<Box> l1 = {
        Box{ivec3(3, 4, 1)} + ivec3(1, 1, 0),
        Box{ivec3(6, 6, 1)} + ivec3(-3, -3, 0),
        Box{ivec3(3, 1, 1)} + ivec3(-5, 0, 0),
        Box{ivec3(5, 1, 1)} + ivec3(-7, 2, 0)
    };

    Box::show_2d_cells(l1);
    Box::show_2d(l1);

    list<Box> l2 = {
        ivec3(-5, -3, -5) + Box{ivec3(7, 7, 7)},
        ivec3(2, -3, -5) + Box{ivec3(4, 4, 7)},
        ivec3(6, -3, -1) + Box{ivec3(3, 3, 3)},
        ivec3(8, -3, -2) + Box{ivec3(1, 1, 1)},
        ivec3(-9, 0, 0) + Box{ivec3(1, 5, 3)}
    };

    Box::show_oblique(l2, 12);

    cout << '\n';

    int col = 0;
    for (auto i: (ivec3(1, 2, 3) + Box{ivec3(2, 3, 4)}).coords())
        cout << i << (++col % 4 ? '\t' : '\n');
    col = 0;
    for (auto i: (ivec3(1, 2, 3) + Box{ivec3(4, 5, 6)}).boxes(Box{ivec3(3)}))
        cout << i << (++col % 2 ? '\t' : '\n');
}
