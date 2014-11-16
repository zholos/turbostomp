#ifndef CORE_TOOLS_H
#define CORE_TOOLS_H

#include "sea.h"

#include <pgamecc.h>

using pgamecc::dloc;


class Bolt : public Sprite {
    double velocity;

public:
    Bolt(double velocity) : velocity(velocity) {}

    void placed();

    interaction collide(Tile&);
    interaction collide(Sprite&);

    double radius() const { return 3; }

    void render(SpriteStream&);
};


struct Ball : Sprite {
    ode::SphereRef sphere;

public:
    void placed();
    void removing();

    interaction collide(Tile&);

    double bounciness() const { return .8; }

    void render(SpriteStream&);
};


#endif
