#ifndef LEVELS_CRAFT_H
#define LEVELS_CRAFT_H

#include "control.h"
#include "mesh.h"
#include "ode.h"
#include "sea.h"

#include <experimental/optional>

#include <pgamecc.h>

using std::experimental::optional;
using pgamecc::dloc;


struct Craft : Sprite {
    static const Mesh& mesh();

    Craft();

    void placed();
    void removing();

    void before_tick();

    void init_controls(Controls&);
    void input(Controls&);

    void render(SpriteStream&);

    struct Joints {
        ode::LMotorJoint thruster, strafer;
        ode::AMotorJoint turner, pitcher, roller;

        Joints(ode::World& w) :
            thruster(w), strafer(w), turner(w), pitcher(w), roller(w) {}
    };
    optional<Joints> joints;

    bool engine = true;
    int blast_cooldown = 0;
    int ball_cooldown = 0;

    struct Thruster {
        dloc l;
        double k;
        bool hit;
        double distance;
    } thrusters[37];
};


#endif
