#include "window.h"

#include "debug.h"
#include "effect.h"
#include "level.h"
#include "render.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

using std::cout;
using std::fixed;
using std::setprecision;
using std::runtime_error;
using std::ostringstream;

using namespace pgamecc;
using namespace pgamecc::key;


class FPSOverlay : public ui::Layer {
    ui::Label* label;
    Window& window;

public:
    FPSOverlay(Window& window) :
        window(window)
    {
        label = create<ui::Label>();
    }

    void layout() override {
        label->place_at(dvec2(0, 0), dvec2(1, 1) * em);
    }

    void step() {
        ostringstream text;
        text << fixed << setprecision(1) << "FPS: " << window.fps() << '\n';
        label->set_text(text.str());
    }
};


Window::Window()
{
    set_title("Turbostomp");
    fullscreen();

    ode::init();

    // TODO: set window title to include level name
    assert(!Level::catalogue::empty());
    level.reset(Level::catalogue::first());
    level->generate();

    fps_overlay = create_layer<FPSOverlay>(*this);
    create_layer<ui::WindowControlLayer>(*this);
}


Window::~Window()
{
    // TODO: check why this crashes, perhaps some ODE objects still exist
    //ode::done();
}


void
Window::background_input_key(bool press, int key, int mods)
{
    bool ctrl = mods & mod_control,
          alt = mods & mod_alt,
        shift = mods & mod_shift;

    if (press && key == '`')
        fps_overlay->active ^= 1;
    else if (press && key >= '0' && key <= '9')
        debug::number = key - '0';
    else if (press && key >= key_f1 && key <= key_f12)
        debug::toggle[key-key_f1] ^= 1;
    else
        level->controls.input_key(press, key, mods);
}


void
Window::background_step()
{
    level->step();
    BoxEffect::step_all();
}


void Window::background_render_init() { renderer.reset(new Renderer()); }
void Window::background_render_done() { renderer.reset(); }

void
Window::background_render()
{
    renderer->render(size(), level->camera, level->grid, level->sea);
}
