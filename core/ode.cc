#include "ode.h"

#include "mesh.h"

#include <ode/ode.h>

using std::move;

using namespace ode;


//
// library initialization
//

void
ode::init()
{
    // FIXME: use dInitODE2()
    dInitODE();

    // TODO: check for ODE_EXT_trimesh if needed
}

void
ode::done()
{
    dCloseODE();
}



//
// data structures
//

//
// ode::Mass
//

static dvec3
dvec3_from_d(const dVector3 p)
{
    return dvec3(p[0], p[1], p[2]);
}

static dquat
dquat_from_d(const dQuaternion q)
{
    return dquat(q[0], q[1], q[2], q[3]);
}


Mass::Mass() :
    mass(new dMass)
{
    dMassSetZero(mass.get());
}

Mass::Mass(double sphere_radius, double density) :
    mass(new dMass)
{
    dMassSetSphere(mass.get(), density, sphere_radius);
}

Mass::Mass(const TriMeshRef& geom, double density) :
    mass(new dMass)
{
    dMassSetTrimesh(mass.get(), density, geom.id());
}

Mass::Mass(const Mass& r) :
    mass(new dMass(*r.mass.get()))
{
}

Mass&
Mass::operator=(const Mass& r)
{
    mass.reset(Mass(r).mass.release());
    return *this;
}

Mass::Mass(Mass&& r) :
    mass(move(r.mass))
{
}

Mass&
Mass::operator=(Mass&& r)
{
    mass = move(r.mass);
    return *this;
}

Mass::~Mass()
{
}


glm::dvec3
Mass::center() const
{
    const dMass* m = mass.get();
    assert(m);
    return dvec3_from_d(m->c);
}


Mass&
Mass::set_total(double total)
{
    dMassAdjust(mass.get(), total);
    return *this;
}


Mass&
Mass::translate(glm::dvec3 v)
{
    dMassTranslate(mass.get(), v.x, v.y, v.z);
    return *this;
}


Mass&
Mass::operator+=(const Mass& r)
{
    dMassAdd(mass.get(), r.mass.get());
    return *this;
}



//
// ode::Contact
//

dvec3
Contact::position() const
{
    return dvec3_from_d(contact.geom.pos);
}

dvec3
Contact::normal() const
{
    return dvec3_from_d(contact.geom.normal);
}

double
Contact::depth() const
{
    return contact.geom.depth;
}


void
Contact::set_mu(double mu)
{
    contact.surface.mu = mu;
}

void
Contact::set_bounce(double bounce)
{
    contact.surface.mode |= dContactBounce;
    contact.surface.bounce = bounce;
    contact.surface.bounce_vel = 0;
}

void
Contact::set_contact_approx_1()
{
    contact.surface.mode |= dContactApprox1;
}

#ifndef NDEBUG
ostream&
ode::operator<<(ostream& os, const Contact& c) {
    using pgamecc::operator<<;
    return os << "ode::Contact(p=" << dvec3_from_d(c.contact.geom.pos) <<
                            ", n=" << dvec3_from_d(c.contact.geom.normal) <<
                            ", depth=" << c.contact.geom.depth << ')';
}
#endif



//
// dynamics simulation
//

//
// ode::Body
//

BodyRef::ID
BodyRef::create(const WorldRef& world)
{
    return dBodyCreate(world.id());
}

void
BodyRef::destroy()
{
    dBodyDestroy(id());
}

void
BodyRef::set_data(void* data)
{
    dBodySetData(id(), data);
}

void*
BodyRef::get_data(dBodyID id)
{
    assert(id);
    return dBodyGetData(id);
}


void
BodyRef::destroy_geoms()
{
    dGeomID next;
    for (dGeomID geom_id = dBodyGetFirstGeom(id()); geom_id; geom_id = next) {
        next = dBodyGetNextGeom(geom_id); // get before destroying
        dGeomDestroy(geom_id);
    }
}


dloc
BodyRef::location() const
{
    return { dvec3_from_d(dBodyGetPosition(id())),
             dquat_from_d(dBodyGetQuaternion(id())) };
}

void
BodyRef::set_location(dloc l)
{
    dBodySetPosition(id(), l.p.x, l.p.y, l.p.z);
    dQuaternion q; q[0] = l.q.w; q[1] = l.q.x; q[2] = l.q.y; q[3] = l.q.z;
    dBodySetQuaternion(id(), q);
}

void
BodyRef::set_linear_velocity(dvec3 v)
{
    dBodySetLinearVel(id(), v.x, v.y, v.z);
}


dvec3
BodyRef::velocity_at(dvec3 at) const
{
    dVector3 velocity;
    dBodyGetRelPointVel(id(), at.x, at.y, at.z, velocity);
    return dvec3_from_d(velocity);
}


Mass
BodyRef::mass() const
{
    Mass mass;
    dBodyGetMass(id(), mass.mass.get());
    return mass;
}

void
BodyRef::set_mass(const Mass& mass)
{
    const dMass* m = mass.mass.get();
    assert(m);
    assert(m->c[0] == 0 && m->c[1] == 0 && m->c[2] == 0);
    dBodySetMass(id(), m);
}

void
BodyRef::set_kinematic()
{
    dBodySetKinematic(id());
}

void
BodyRef::set_gravity_mode(bool influenced)
{
    dBodySetGravityMode(id(), influenced);
}


void
BodyRef::add_force(double force, dloc l)
{
    dvec3 rel = l.q * dvec3(0, 0, -force);
    dBodyAddRelForceAtRelPos(id(), rel.x, rel.y, rel.z, l.p.x, l.p.y, l.p.z);
}


void
BodyRef::operator<<(const GeomRef& geom)
{
    assert(!dGeomGetBody(geom.id()));
    dGeomSetBody(geom.id(), id());
}


#ifndef NDEBUG
ostream&
ode::operator<<(ostream& os, const BodyRef& b) {
    using pgamecc::operator<<;
    return os << "ode::Body(p=" << b.location().p << ')';
}
#endif



//
// ode::TranslatedBody
//

dloc
TranslatedBody::location() const
{
    auto l = Body::location();
    return l - l.q * offset;
}

void
TranslatedBody::set_location(dloc l)
{
    Body::set_location(l + l.q * offset);
}


Mass
TranslatedBody::mass() const
{
    Mass mass = Body::mass();
    mass.translate(offset);
    return mass;
}

void
TranslatedBody::set_mass(const Mass& mass_)
{
    // NOTE: Geom translation is taken into account by functions which do that.
    // Not sure how exactly this would be used more than once
    assert(offset == dvec3());

    Mass mass(mass_);
    offset = mass.center();

    if (offset != dvec3()) {
        // Translate body center to mass center, and then translate geoms and
        // mass in the opposite direction to compensate

        auto v = dvec3_from_d(dBodyGetPosition(id())) + offset;
        dBodySetPosition(id(), v.x, v.y, v.z);

        for (dGeomID geom_id = dBodyGetFirstGeom(id()); geom_id;
                     geom_id = dBodyGetNextGeom(geom_id)) {
            auto u = dvec3_from_d(dGeomGetOffsetPosition(geom_id)) - offset;
            dGeomSetOffsetPosition(geom_id, u.x, u.y, u.z);
        }

        mass.translate(-offset);
    }

    Body::set_mass(mass);
}


dvec3
TranslatedBody::velocity_at(dvec3 at) const
{
    return Body::velocity_at(at - offset);
}


void
TranslatedBody::add_force(double force, dloc l)
{
    Body::add_force(force, l - offset);
}



//
// ode::Joint
//

// no create() - that's for specific types of joints

void
JointRef::destroy()
{
    dJointDestroy(id());
}

void
JointRef::set_data(void* data)
{
    dJointSetData(id(), data);
}

void*
JointRef::get_data(ID id)
{
    assert(id);
    return dJointGetData(id);
}


void
JointRef::attach(const BodyRef* body1, const BodyRef* body2)
{
    dJointAttach(id(), body1 ? body1->id() : 0,
                       body2 ? body2->id() : 0);
}



//
// ode::LMotorJoint and ode::AMotorJoint
//

LMotorJointRef::ID
LMotorJointRef::create(const WorldRef& world)
{
    return dJointCreateLMotor(world.id(), 0);
}

void
LMotorJointRef::set_num_axes(int num)
{
    assert(0 <= num && num <= 3);
    dJointSetLMotorNumAxes(id(), num);
}

void
LMotorJointRef::set_axis(int which, Rel rel, dvec3 axis)
{
    // TODO: for non-global axis, body must already be attached; check this
    assert(0 <= which && which < dJointGetLMotorNumAxes(id()));
    dJointSetLMotorAxis(id(), which, static_cast<int>(rel),
                        axis.x, axis.y, axis.z);
}

void
LMotorJointRef::set_vel(double value)
{
    dJointSetLMotorParam(id(), dParamVel, value);
}

void
LMotorJointRef::set_fmax(double value)
{
    dJointSetLMotorParam(id(), dParamFMax, value);
}



AMotorJointRef::ID
AMotorJointRef::create(const WorldRef& world)
{
    return dJointCreateAMotor(world.id(), 0);
}

void
AMotorJointRef::set_num_axes(int num)
{
    assert(0 <= num && num <= 3);
    dJointSetAMotorNumAxes(id(), num);
}

void
AMotorJointRef::set_axis(int which, Rel rel, dvec3 axis)
{
    // TODO: for non-global axis, body must already be attached; check this
    assert(0 <= which && which < dJointGetAMotorNumAxes(id()));
    dJointSetAMotorAxis(id(), which, static_cast<int>(rel),
                        axis.x, axis.y, axis.z);
}

void
AMotorJointRef::set_vel(double value)
{
    dJointSetAMotorParam(id(), dParamVel, value);
}

void
AMotorJointRef::set_fmax(double value)
{
    dJointSetAMotorParam(id(), dParamFMax, value);
}



//
// ode::JointGroup
//

JointGroupRef::ID
JointGroupRef::create()
{
    return dJointGroupCreate(0); // max_size argument apparently no longer used
}

void
JointGroupRef::destroy()
{
    dJointGroupDestroy(id());
}

void
JointGroupRef::clear()
{
    dJointGroupEmpty(id());
}

ContactJointRef
JointGroupRef::new_contact_joint(const WorldRef& world,
                                         const Contact& contact)
{
    return dJointCreateContact(world.id(), id(), &contact.contact);
}



//
// ode::World
//

WorldRef::ID
WorldRef::create()
{
    return dWorldCreate();
}

void
WorldRef::destroy()
{
    dWorldDestroy(id());
    // this invalidates all bodies
}


void
WorldRef::set_gravity(dvec3 p)
{
    dWorldSetGravity(id(), p.x, p.y, p.z);
}


void
WorldRef::step(double step_size)
{
    dWorldStep(id(), step_size);
}

void
WorldRef::quick_step(double step_size)
{
    dWorldQuickStep(id(), step_size);
}



//
// collision detection
//

//
// ode::Geom
//

void
GeomRef::destroy()
{
    dGeomDestroy(id());
}

void
GeomRef::set_data(void* data)
{
    dGeomSetData(id(), data);
}

void*
GeomRef::get_data(ID id)
{
    assert(id);
    return dGeomGetData(id);
}


dloc
GeomRef::location() const
{
    dQuaternion q;
    dGeomGetQuaternion(id(), q);
    return { dvec3_from_d(dGeomGetPosition(id())), dquat_from_d(q) };
}

void
GeomRef::set_location(dloc l)
{
    dGeomSetPosition(id(), l.p.x, l.p.y, l.p.z);
    dQuaternion q; q[0] = l.q.w; q[1] = l.q.x; q[2] = l.q.y; q[3] = l.q.z;
    dGeomSetQuaternion(id(), q);
}


Body&
GeomRef::body() const
{
    return Body::find(dGeomGetBody(id()));
}


#ifndef NDEBUG
ostream&
ode::operator<<(ostream& os, const GeomRef& b) {
    using pgamecc::operator<<;
    const char* type = "Geom";
    switch (dGeomGetClass(b.id())) {
    case dSphereClass:  type = "Sphere";  break;
    case dBoxClass:     type = "Box";     break;
    case dCapsuleClass: type = "Capsule"; break;
    case dRayClass:     type = "Ray";     break;
    case dTriMeshClass: type = "TriMesh"; break;
    case dFirstSpaceClass ... dLastSpaceClass: type = "Space"; break;
    }
    return os << "ode::" << type << "(p=" << b.location().p << ')';
}
#endif



//
// ode::Sphere, ode::Box and ode::Capsule
//

SphereRef::ID
SphereRef::create(double radius)
{
    return dCreateSphere(0, radius);
}

double
SphereRef::radius() const
{
    assert(dGeomGetClass(id()) == dSphereClass);
    return dGeomSphereGetRadius(id());
}

void
SphereRef::set_radius(double radius)
{
    assert(dGeomGetClass(id()) == dSphereClass);
    dGeomSphereSetRadius(id(), radius);
}


BoxRef::ID
BoxRef::create(dvec3 size)
{
    return dCreateBox(0, size.x, size.y, size.z);
}

dvec3
BoxRef::size() const
{
    assert(dGeomGetClass(id()) == dBoxClass); // ODE checks this too
    dVector3 v;
    dGeomBoxGetLengths(id(), v);
    return dvec3_from_d(v);
}


CapsuleRef::ID
CapsuleRef::create(double radius, double length)
{
    return dCreateCapsule(0, radius, length);
}



//
// ode::Ray
//

void
ode::detail::ray_near_callback(void* data, dGeomID ray_id, dGeomID geom_id)
{
    assert(dGeomGetClass(ray_id) == dRayClass);
    optional<Ray::Hit>& result = *static_cast<optional<Ray::Hit>*>(data);

    // NOTE: there may be more than one collision with a TriMesh depending on
    // ray parameters
    const int max_points = 2;
    dContactGeom points[max_points];
    int num_points = dCollide(ray_id, geom_id, max_points,
                              points, sizeof points);
    for (int i = 0; i < num_points; i++)
        if (!result || points[i].depth < result->distance)
            result = Ray::Hit{dvec3_from_d(points[i].pos),
                              dvec3_from_d(points[i].normal),
                              points[i].depth,
                              GeomRef(geom_id)};
}

optional<Ray::Hit>
RayRef::hit(const GeomRef& geom) const
{
    optional<Ray::Hit> result;
    dSpaceCollide2(id(), geom.id(), &result, ode::detail::ray_near_callback);
    return result;
}


RayRef::ID
RayRef::create(double length)
{
    return dCreateRay(0, length);
}

Ray::Ray(double length, dvec3 position, dvec3 direction) :
    Owned(length)
{
    dGeomRaySet(id(), position.x, position.y, position.z,
                direction.x, direction.y, direction.z);

    // These parameters should ensure there is only one collision reported for
    // TriMesh, but still be prepared to handle more than one just in case
    dGeomRaySetParams(id(), /*FirstContact:*/false, /*BackfaceCull:*/true);
    dGeomRaySetClosestHit(id(), true);
}

Ray::Ray(double length, dloc location) :
    Ray(length, location.p, location.q * dvec3(0, 0, -1))
{
}


//
// ode::TriMeshData and ode::TriMesh
//

TriMeshDataRef::ID
TriMeshDataRef::create()
{
    return dGeomTriMeshDataCreate();
}

void
TriMeshDataRef::destroy()
{
    dGeomTriMeshDataDestroy(id());
}


TriMeshData::TriMeshData(const Mesh& mesh)
{
    auto verts = mesh.verts();
    v.reset(new float[3*verts.size()]);
    auto vi = &v[0];
    for (auto m: verts) {
        *vi++ = m.x;
        *vi++ = m.y;
        *vi++ = m.z;
    }

    auto tris = mesh.tris();
    f.reset(new int[3*tris.size()]);
    auto fi = &f[0];
    for (auto t: tris) {
        *fi++ = t[0];
        *fi++ = t[1];
        *fi++ = t[2];
    }

    dGeomTriMeshDataBuildSingle(id(),
                                v.get(), 3*sizeof v[0], verts.size(),
                                f.get(), 3*tris.size(), 3*sizeof f[0]);
}


TriMeshRef::ID
TriMeshRef::create(const TriMeshDataRef& data)
{
    return dCreateTriMesh(0, data.id(), nullptr, nullptr, nullptr);
}



//
// ode::Space
//

SpaceRef::ID
SpaceRef::create()
{
    // inheritance of dxSpace from dxGeom is not exported, so need cast
    return (dGeomID)dHashSpaceCreate(0);
}

// dSpaceDestroy just calls dGeomDestroy


SpaceRef&
SpaceRef::operator<<(const GeomRef& geom)
{
    dSpaceAdd((dSpaceID)id(), geom.id());
    return *this;
}


Sphere
SpaceRef::new_sphere(double radius)
{
    Sphere s(radius);
    operator<<(s);
    return s;
}

Box
SpaceRef::new_box(dvec3 size)
{
    Box b(size);
    operator<<(b);
    return b;
}


Capsule
SpaceRef::new_capsule(double radius, double length)
{
    Capsule c(radius, length);
    operator<<(c);
    return c;
}


TriMesh
SpaceRef::new_trimesh(const TriMeshData& data)
{
    TriMesh m(data);
    operator<<(m);
    return m;
}


void
SpaceRef::collide(ode::detail::CollideCallback callback)
{
    // inheritance of dxSpace from dxGeom is not exported, so need cast
    dSpaceCollide((dSpaceID)id(), &callback, ode::detail::near_callback);
}



//
// collision handling for Geom and Space
//

void
ode::contacts(const GeomRef& geom1, const GeomRef& geom2,
              ode::detail::ContactsCallback callback)
{
    const int max_points = 32; // not too large to waste space,
                               // not more than (1<<16)-1
    dContactGeom points[max_points];

    int num_points = dCollide(geom1.id(), geom2.id(), max_points,
                              points, sizeof *points);
    for (int i = 0; i < num_points; i++) {
        dContact contact = { {}, points[i], {} };
        callback(Contact{contact});
    }
}


void
ode::detail::near_callback(void* data, dGeomID id1, dGeomID id2)
{
    ode::detail::CollideCallback& callback =
        *static_cast<ode::detail::CollideCallback*>(data);
    const ode::Geom* geom1 = ode::Geom::try_find(id1);
    const ode::Geom* geom2 = ode::Geom::try_find(id2);
    const ode::GeomRef ref1(id1);
    const ode::GeomRef ref2(id2);
    callback(geom1 ? *geom1 : ref1,
             geom2 ? *geom2 : ref2);
}


void
ode::collide(const GeomRef& geom1, const GeomRef& geom2,
             ode::detail::CollideCallback callback)
{
    dSpaceCollide2(geom1.id(), geom2.id(), &callback,
                   ode::detail::near_callback);
}
