#ifndef CORE_PAINT_H
#define CORE_PAINT_H

#include "grid.h"

namespace paint {

void sphere(View, Tile);
void sphere_smooth(View, Tile);
void cylinder(View, Tile);
void tree(View);
void heightmap(View, pgamecc::Image<int>, Tile);
void heightmap_smooth(View, pgamecc::Image<int>, Tile);
void rolling_hills(View, Tile);
void rolling_hills_smooth(View, Tile);
void trees(View);

}

#endif
