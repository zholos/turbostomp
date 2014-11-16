#include "control.h"

#include <algorithm>
#include <cassert>

#include <pgamecc.h>

using std::min;

using namespace pgamecc::key;


void
Controls::add_key(int control, int key, int sign)
{
    auto r = keys.emplace(key, Key{control, sign});
    assert(r.second);
}

void
Controls::add_control(int control, Control::Type type, int state)
{
    auto r = controls.emplace(control, Control{type, state, false});
    assert(r.second);
}

void
Controls::axis(int control, int minus_key, int plus_key)
{
    add_key(control, minus_key, -1);
    add_key(control, plus_key, 1);
    add_control(control, Control::Type::axis);
}

void
Controls::slider(int control, int minus_key, int plus_key, int state)
{
    add_key(control, minus_key, -1);
    add_key(control, plus_key, 1);
    add_control(control, Control::Type::slider, state);
}

void
Controls::trigger(int control, int key)
{
    add_key(control, key);
    add_control(control, Control::Type::trigger);
}

void
Controls::release(int control, int key)
{
    add_key(control, key);
    add_control(control, Control::Type::release);
}

void
Controls::toggle(int control, int key, bool state)
{
    add_key(control, key);
    add_control(control, Control::Type::toggle, state);
}


int
Controls::operator[](int _control)
{
    Control& control = controls.at(_control);
    switch (control.type) {
    case Control::Type::release:
        if (control.state) {
            control.state--;
            return 1;
        } else
            return 0;
    default:
        return control.state;
    }
}


void
Controls::input_key(bool press, int _key, int mods)
{
    auto it = keys.find(_key);
    if (it == keys.end())
        return;

    const Key& key = it->second;
    Control& control = controls.at(key.control);
    switch (control.type) {
    case Control::Type::axis:
        if (press)
            control.state = key.sign;
        else
            if (control.state == key.sign)
                control.state = 0;
        break;
    case Control::Type::slider:
        if (control.pressed != press) {
            if (press)
                control.state += key.sign;
            control.pressed = press;
        }
        break;
    case Control::Type::trigger:
        control.state = press;
        break;
    case Control::Type::release:
        if (control.pressed != press) {
            if (press)
                control.state =
                    min(100, control.state + (mods & mod_shift ? 10 : 1));
            control.pressed = press;
        }
        break;
    case Control::Type::toggle:
        if (control.pressed != press) {
            if (press)
                control.state ^= 1;
            control.pressed = press;
        }
        break;
    }
}
