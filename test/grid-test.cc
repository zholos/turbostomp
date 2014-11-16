#include "grid.h"

Grid grid;

void show()
{
    list<Box> boxes;
    View(grid).each_tile([&] (SBox s, Tile) {
        boxes.push_back(s);
    });
    Box::show_oblique(boxes, grid.size() * 3/2);
    std::cout << '\n';
}


void letter_f(View v)
{
    v.cut();
    v = v.clip(Box({4, 5, 1})).base();
    v.clip(Box({1, 5, 1})).fill({});
    v.rotate(irot::rotate_z()).base().clip(Box({1, 4, 1})).fill({});
    v.translate({0, 2, 0}).clip(Box({3, 1, 1})).fill({});
}

int
main()
{
    grid = Grid(4);

    View(grid).fill({});
    View v = View(grid).clip(ivec3(1, 1, 1) + SBox{3});

    v.cut();
    v.base()[ivec3(2, 0, 0)].fill({});
    show();

    v.cut();
    v.translate(ivec3(3, 1, 2))[ivec3(0, 0, 0)].fill({});
    show();

    v.cut();
    v.center()[ivec3(0, 0, 0)].fill({});
    show();

    v.cut();
    v.base().clip(SBox{2}).center()[ivec3(0, 0, 0)].fill({});
    show();

    v.cut();
    v.base().clip(SBox{1}).center()[ivec3(0, 0, 0)].fill({});
    show();

    v.cut();
    auto w = v.base().clip(Box({3, 1, 3}));
    w.fill({});
    w.rotate(irot::rotate_y()).base()[ivec3(0, 0, 0)].cut();
    show();

    grid = Grid(8);

    letter_f(View(grid));
    show();

    letter_f(View(grid).translate({0, 0, 3}));
    show();

    letter_f(View(grid).rotate(irot::rotate_x()).base());
    show();

    letter_f(View(grid).rotate(irot::rotate_y() * irot::flip_x()).base());
    show();

    grid.ctop().show_text();
}
