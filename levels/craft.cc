#include "craft.h"

#include "assets.h"
#include "debug.h"
#include "mesh.h"
#include "render.h"
#include "tools.h"

#include <utility>

using std::move;
using std::experimental::nullopt;
using namespace pgamecc::key;


static const auto transformation = glm::scale(glm::dvec3(.5));

const Mesh&
Craft::mesh()
{
    static const Mesh mesh(meshes["craft.obj"], transformation);
    return mesh;
}


Craft::Craft()
{
    auto init_thruster = [&] (dloc l, double k) -> Thruster {
        // apply mesh transform to position only, not orientation, e.g. to keep
        // it pointing straight down even if mesh is rotated slightly
        l.p = dvec3(transformation * dvec4(l.p, 1));
        return { l, k };
    };
    auto down = glm::angleAxis(glm::radians(-90.), dvec3(1, 0, 0));
    for (int i = 0; i < 37; i++) {
        auto offset = glm::rotateY(dvec3(0, 0, 3.*((i+11)/12)),
                                   glm::radians(360. * ((i+11)%12)/12));
        thrusters[i] = init_thruster({ offset, down }, .05);
    }
}


void
Craft::placed()
{
    assert(island);

    // assuming mesh data can be shared between ODE worlds
    static ode::TriMeshData ode_mesh(mesh());

    auto geom = island->sprite_space.new_trimesh(ode_mesh);

    ode::Mass mass(geom);
    mass.set_total(1);

    body <<= move(geom);
    body.set_mass(mass);


    joints.emplace(island->world);

    joints->thruster.attach(&body, nullptr);
    joints->thruster.set_axes(ode::LMotorJoint::Rel::first, dvec3(0, 0, -1));

    joints->strafer.attach(&body, nullptr);
    joints->strafer.set_axes(ode::LMotorJoint::Rel::first, dvec3(1, 0, 0));

    joints->turner.attach(&body, nullptr);
    joints->turner.set_axes(ode::AMotorJoint::Rel::first, dvec3(0, -1, 0));

    joints->pitcher.attach(&body, nullptr);
    joints->pitcher.set_axes(ode::AMotorJoint::Rel::first, dvec3(1, 0, 0));

    joints->roller.attach(&body, nullptr);
    joints->roller.set_axes(ode::AMotorJoint::Rel::first, dvec3(0, 0, -1));
}


void
Craft::removing()
{
    joints = nullopt;
}


void
Craft::before_tick()
{
    if (!engine)
        return;

    const auto right = glm::angleAxis(glm::radians(-90.), dvec3(0, 1, 0));

    dloc bl = body.location();

    // Thuster repel along -z axis (also dampens it for stability)
    // and resist movement along +/-x axis (to prevent drifting)

    double level = 5;

    for (auto& t: thrusters) {
        ode::Ray ray(level*2, bl * t.l);
        // TODO: allow gliding over sprites as well as voxels
        auto hit = ray.hit(island->voxel_space);
        if (t.hit = bool(hit)) {
            t.distance = hit->distance;

            auto pv = body.velocity_at(t.l.p);

            // repel along -z axis, with dampening for stability
            double zv = glm::dot(pv, t.l.q * dvec3(0, 0, -1));
            body.add_force(-t.k * (2*pow(level - t.distance, 3) + zv), t.l);
        }
    }

    // TODO: dampen spinning
}


void
Craft::init_controls(Controls& controls)
{
    controls.axis(0, 'S', 'W');
    controls.axis(1, 'A', 'D');
    controls.axis(2, key_up, key_down); // correctly inverted
    controls.axis(3, key_left, key_right);
    controls.trigger(4, key_left_control);
    controls.release(5, 'B');
    controls.toggle(6, 'R', true);
}


void
Craft::input(Controls& controls)
{
    assert(joints); // shouldn't be dormant

    int thrust = controls[0];
    joints->thruster.set_vel(thrust ? thrust > 0 ? 25 : -5 : 0);
    joints->thruster.set_fmax(thrust ? 30 : 0);

    int yaw = controls[1];
    joints->strafer.set_vel(yaw * 10);
    joints->strafer.set_fmax(yaw ? 30 : 0);

    int pitch = controls[2];
    joints->pitcher.set_vel(pitch * 20);
    joints->pitcher.set_fmax(pitch ? 10 : 1);

    int roll = controls[3];
    joints->turner.set_vel(roll * 15);
    joints->turner.set_fmax(roll ? 10 : 5); // small dampening component

    if (blast_cooldown)
        blast_cooldown--;
    else
        if (controls[4]) {
            blast_cooldown = 5;
            dloc l = location();
            auto blaster = [&] (dvec3 p) {
                p = dvec3(transformation * dvec4(p, 1));
                island->create<Bolt>(l + l.q * p, 75.);
            };
            blaster(dvec3(-.2, 0, -5));
            blaster(dvec3( .2, 0, -5));
        }

    if (ball_cooldown)
        ball_cooldown--;
    else
        if (controls[5]) {
            ball_cooldown = 25;

            dloc l = location();
            auto p = dvec3(transformation * dvec4(0, -2, 0, 1));
            Ball* ball = island->create<Ball>(l + l.q * p);
            ball->body.set_linear_velocity(body.velocity_at(p));
        }

    engine = controls[6];
}


void
Craft::render(SpriteStream& stream)
{
    stream.push_mesh(&mesh(), location());
    if (!engine)
        return;
    if (debug::toggle[0])
        for (auto& t: thrusters)
            stream.push_thruster(location() * t.l, t.hit, t.distance);
}
