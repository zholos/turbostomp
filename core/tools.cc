#include "tools.h"

#include "grid.h"
#include "render.h"

#include <utility>

using std::move;


//
// Bolt
//

void
Bolt::placed()
{
    body.set_kinematic();
    body.set_linear_velocity(body.location().q * dvec3(0, 0, -75));
    // TODO: velocity depends on craft velocity
    body <<= island->sprite_space.new_capsule(.1, 2);
}

Bolt::interaction
Bolt::collide(Tile& t)
{
    assert(t); // shouldn't collide with empty
    t = t.hit();
    return interaction::destroyed;
}

Bolt::interaction
Bolt::collide(Sprite&)
{
    // TODO: only ignore for originating craft for first few ticks
    return interaction::ignore;
}

void
Bolt::render(SpriteStream& stream)
{
    stream.push_bolt(location());
}


//
// Ball
//

void
Ball::placed()
{
    ode::Sphere geom = island->sprite_space.new_sphere(2/3.);
    sphere = geom;
    body <<= move(geom);
}

void
Ball::removing()
{
    sphere = ode::SphereRef();
}


Ball::interaction
Ball::collide(Tile& t)
{
    assert(t); // shouldn't collide with empty
    t = t.color((t.color() + 1) % 32);
    return interaction::hit; // TODO: destroyed
}


void
Ball::render(SpriteStream& stream)
{
    stream.push_ball(location(), sphere.radius());
}
