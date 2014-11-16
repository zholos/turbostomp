#include "demo.h"

#include "craft.h"
#include "paint.h"

#include <memory>
#include <utility>

using std::move;
using std::unique_ptr;

using namespace paint;


static Level::catalogue demo_entry(100, "Demo", [] { return new DemoLevel; });


void
DemoLevel::generate()
{
    grid = Grid(256);

    auto v = View(grid);
    v.fill(Tile{}.color({15, 10, 0}));

    v = v.center().clip_up();
    v.cut();
    rolling_hills_smooth(
        v.translate(ivec3(0, 7, 0)).
            rotate(irot::flip_y()).clip_up().rotate(irot::flip_y()).base(),
        Tile{}.color({0, 24, 15}));
    trees(v);

    craft = sea.create<Craft>(v.location({dvec3(0, 10, 3)}));
    craft->init_controls(controls);
}


void
DemoLevel::before_step()
{
    craft->input(controls);
}

void
DemoLevel::after_step()
{
    // TODO: only really needed if the level is being displayed
    auto l = craft->location();
    camera.track(l * dvec3(0, 10, 20), // camera position behind craft
                 l * dvec3(0, 5, 0), // watch slightly above craft,
                 dvec3(0, 1, 0)); // fixed up direction for now
}
