#ifndef CORE_WINDOW_H
#define CORE_WINDOW_H

#include <pgamecc.h>
#include <pgamecc/ui.h>

#include <memory>

using std::unique_ptr;

class Renderer;
class Level;


class Window : public pgamecc::ui::WindowBase {
    unique_ptr<Renderer> renderer;
    unique_ptr<Level> level;
    pgamecc::ui::Layer* fps_overlay;

public:
    Window();
    ~Window();

    void background_input_key(bool press, int key, int mods);

    void background_step();

    void background_render_init();
    void background_render_done();
    void background_render();
};

#endif
