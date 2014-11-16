#ifndef CORE_LEVEL_H
#define CORE_LEVEL_H

#include "camera.h"
#include "control.h"
#include "grid.h"
#include "sea.h"

#include <functional>
#include <list>
#include <string>

using std::function;
using std::list;
using std::string;

class Camera;
class Controls;


// A level encompasses the voxel grid and the dynamic object simulation. It
// implements the overall game mechanics.

class Level {
protected:
    Grid grid;
    Sea sea;
    Camera camera;
    Controls controls;

public:
    virtual void generate() = 0;
    virtual void before_step() = 0;
    virtual void after_step() = 0;
    void step();

    struct catalogue {
        catalogue(int priority, string name, function<Level*()> factory);

        struct Entry {
            string name;
            function<Level*()> factory;
        };
        static bool empty();
        static list<Entry> all();
        static Level* first();
    };

#ifndef NDEBUG
    View debug_view() { return View(grid); }
#endif

    friend class Window;
};


#endif
