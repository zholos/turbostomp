#ifndef CORE_CONTROL_H
#define CORE_CONTROL_H

#include <map>

using std::map;


// Control abstraction, obtained by Window from keyboard and mouse input, and
// used by Sprites. The struct is owned by Level and mutated by Craft
// (e.g. consuming presses).

class Controls {
    struct Key {
        int control;
        int sign;
    };
    map<int, const Key> keys; // keycode lookup

    struct Control {
        enum class Type {
            axis,
            slider,
            trigger,
            release,
            toggle
        } type;
        int state;
        bool pressed;
    };
    map<int, Control> controls; // map because index space may be sparse

    void add_key(int control, int key, int sign = 0);
    void add_control(int control, Control::Type type, int state = 0);

public:
    void axis(int control, int minus_key, int plus_key);
    void slider(int control, int minus_key, int plus_key, int state = 0);
    void trigger(int control, int key);
    void release(int control, int key);
    void toggle(int control, int key, bool state = false);

    int operator[](int control);

    void input_key(bool press, int key, int mods);
};


#endif
