#ifndef CORE_ODE_H
#define CORE_ODE_H

#include <pgamecc.h>

#include <functional>
#include <iostream>
#include <memory>
#include <experimental/optional>
#include <utility>

using std::exchange;
using std::ostream;
using std::unique_ptr;
using std::forward;
using std::experimental::optional;

using pgamecc::dvec3;
using pgamecc::dquat;
using pgamecc::dloc;

class Mesh;


// Implementation details from ode/common.h so we don't have include ode/ode.h:
// IDs are really pointers, and it's more convenient to treat them as such
struct dxBody;
struct dxWorld;
struct dxJoint;
struct dxJointGroup;
struct dxGeom;
struct dxTriMeshData;

struct dMass;
struct dContact;


namespace ode {

//
// library initialization
//

void init();
void done();


//
// data structures
//

class TriMeshRef;
class GeomBody;
class Geom;

class Mass {
    unique_ptr<dMass> mass;

public:
    // all implemented where dMass is complete
    Mass(); // zero
    Mass(const Mass&);
    Mass& operator=(const Mass& r);
    Mass(Mass&& r);
    Mass& operator=(Mass&& r);
    ~Mass();

    Mass(double sphere_radius, double density = 1);
    Mass(const TriMeshRef&, double density = 1);

    dvec3 center() const;

    Mass& set_total(double total);

    Mass& translate(dvec3);

    Mass& operator+=(const Mass&);
    Mass operator+(const Mass& r) const { Mass m(*this); return m += r; }

    friend class BodyRef;
};


// mutator for dContact, which is created on the stack in Geom::contacts

class Contact {
    dContact& contact;

public: // TODO: private
    explicit Contact(dContact& contact) : contact(contact) {}

public:
    dvec3 position() const;
    dvec3 normal() const;
    double depth() const;

    void set_mu(double);
    void set_bounce(double);
    void set_contact_approx_1();

    friend class JointGroupRef;

#ifndef NDEBUG
    friend ostream& operator<<(ostream&, const Contact&);
#endif
};


namespace detail {

// There are two kinds of ODE wrappers: non-owning, such as GeomRef, and owning,
// such as Geom. GeomRef defines all the functionality for using the wrapped
// object, and Geom inherits from it (an owned Geom can be used anywhere a
// non-owned GeomRef can). The owning class uses a template to expose the
// constructor and destroy method, which is actually defined in the non-owning
// class for convenience.

template<typename ID_>
struct Object_ {
private:
    ID_ _id;

protected:
    typedef ID_ ID;
    typedef Object_ Object;

    // get_data() is exposed only as find() to ensure that when objects are
    // moved, the data is updated automatically. It's easier for user code to
    // use the appropriate cast from ode::Body to user object than to ensure the
    // data member is updated to point to the correct user object.

    // TODO: use templates to disable find() if these don't exist in derived
    void set_data(void*) {}
    static void* get_data(ID) { assert(false); }

    // Only create with a valid ID in most cases (but there may be a zero ID in
    // an object that was moved from).
    Object_() : _id() {} // TODO: try to eliminate
    Object_(ID id) : _id(id) { assert(_id); }
    ID id() const { assert(_id); return _id; }
    bool has_id() const { return _id; }

    template<typename Derived, typename Ref> friend class Owned_;
};

template<typename Derived, typename Ref>
struct Owned_ : Ref {
protected:
    using typename Ref::ID;
    using Ref::_id;
    using Ref::destroy;
    using Ref::set_data;
    using Ref::get_data;
    typedef Owned_ Owned;

    // Constructor from ID is hidden, semantic constructors using create() are
    // exposed.
    template<typename... Args>
    Owned_(Args&&... args) : Ref(Ref::create(forward<Args>(args)...)) {
        set_data(this);
    }

    // Used by some derived classes to construct from an equivalent ID from
    // another type, but can't hide it from user code, so add protected tag
    // argument
    struct take_tag {};
    Owned_(ID id, take_tag) : Ref(id) { set_data(this); }

    ID release_id() && {
        assert(_id);
        set_data(nullptr);
        return exchange(_id, ID(0));
    }

    static Derived* try_find(ID id) {
        return static_cast<Derived*>(get_data(id));
    }

    static Derived& find(ID id) {
        auto derived = try_find(id);
        assert(derived);
        return *derived;
    }

    // noncopyable
    Owned_(Owned_&& r) : Ref(r._id) { r._id = 0; if (_id) set_data(this); }
    Owned_& operator=(Owned_&& r) {
        if (_id) destroy();
        if (this != &r) { _id = r._id; r._id = 0; if (_id) set_data(this); }
        return *this;
    }
    ~Owned_() { if (_id) destroy(); } // ODE functions need a valid ID

    friend class ode::GeomBody; // for release_id()
    friend class ode::Geom; // for release_id()
};

}


//
// dynamics simulation
//

class WorldRef;
class GeomRef;

// TODO: inherit privately here and elsewhere
struct BodyRef : detail::Object_<dxBody*> {
protected:
    using Object::Object;
    static ID create(const WorldRef&); // could get WorldRef or World
    void destroy();
    void set_data(void*);
    static void* get_data(ID);

    void destroy_geoms();

public:
    dloc location() const;
    void set_location(dloc);

    void set_linear_velocity(dvec3);

    dvec3 velocity_at(dvec3 at) const;

    Mass mass() const;
    void set_mass(const Mass&);
    void set_kinematic(); // infinite mass
    void set_gravity_mode(bool influenced);

    // takes ownership
    void operator<<(const GeomRef& geom);

    void add_force(double force, dloc); // -z in dloc relative to body frame

    friend class JointRef;

#ifndef NDEBUG
    friend ostream& operator<<(ostream&, const BodyRef&);
#endif
};

struct Body : detail::Owned_<Body, BodyRef> {
    using Owned::Owned;

    friend class GeomRef;
};

struct GeomBody : Body {
    using Body::Body;
    ~GeomBody() {
        if (has_id()) // may have been moved from
            destroy_geoms();
    }

    // required because of destructor:
    GeomBody(GeomBody&&) = default;
    GeomBody& operator=(GeomBody&&) = default;

    template<typename Geom>
    void operator<<=(Geom&& geom) { operator<<(std::move(geom).release_id()); }
};


// ODE bodies are located at their center of mass, which may be at an offset
// from the mesh origin. This class transforms all coordinates in the underlying
// body functions.
struct TranslatedBody : GeomBody {
protected:
    dvec3 offset = {}; // center of mass relative to mesh
                       // ODE body (center of mass) = this body + offset

public:
    using GeomBody::GeomBody;

    dloc location() const;
    void set_location(dloc);

    dvec3 velocity_at(dvec3 at) const;

    Mass mass() const;
    void set_mass(const Mass&);

    void add_force(double force, dloc); // -z in dloc relative to body frame
};


struct JointRef : detail::Object_<dxJoint*> {
protected:
    using Object::Object;
    void destroy();
    void set_data(void*);
    static void* get_data(ID);

public:
    void attach(const BodyRef*, const BodyRef*);
};

struct ContactJointRef : JointRef {
protected:
    using JointRef::JointRef;
    // no real need to create it outside joint group

    friend class JointGroupRef;
};

struct LMotorJointRef : JointRef {
protected:
    using JointRef::JointRef;
    static ID create(const WorldRef&);

public:
    enum class Rel { global, first, second };

    void set_num_axes(int);
    void set_axis(int which, Rel rel, dvec3 axis);
    void set_axes(Rel rel0, dvec3 axis0) {
        set_num_axes(1); set_axis(0, rel0, axis0);
    }

    void set_vel(double);
    void set_fmax(double);
};

struct LMotorJoint : detail::Owned_<LMotorJoint, LMotorJointRef> {
    using Owned::Owned;
};

struct AMotorJointRef : JointRef {
protected:
    using JointRef::JointRef;
    static ID create(const WorldRef&);

public:
    enum class Rel { global, first, second };

    void set_num_axes(int);
    void set_axis(int which, Rel rel, dvec3 axis);
    void set_axes(Rel rel0, dvec3 axis0) {
        set_num_axes(1); set_axis(0, rel0, axis0);
    }

    void set_vel(double);
    void set_fmax(double);
};

struct AMotorJoint : detail::Owned_<AMotorJoint, AMotorJointRef> {
    using Owned::Owned;
};


struct JointGroupRef : detail::Object_<dxJointGroup*> {
protected:
    using Object::Object;
    static ID create();
    void destroy();

public:
    void clear();

    ContactJointRef new_contact_joint(const WorldRef&,
                                      const Contact&); // owned by group
};

struct JointGroup : detail::Owned_<JointGroup, JointGroupRef> {
protected:
    WorldRef& world; // for convenience in calling new_* functions

public:
    JointGroup(WorldRef& world) : world(world) {}

    ContactJointRef new_contact_joint(const Contact& contact) {
        return Owned::new_contact_joint(world, contact);
    }
};


struct WorldRef : detail::Object_<dxWorld*> {
protected:
    using Object::Object;
    static ID create();
    void destroy();

public:
    void set_gravity(dvec3);

    Body new_body() { return Body(*this); }
    JointGroup new_joint_group() { return JointGroup(*this); }

    void step(double step_size);
    void quick_step(double step_size);

    friend class BodyRef;
    friend class JointGroupRef;
    friend class LMotorJointRef;
    friend class AMotorJointRef;
};

struct World : detail::Owned_<World, WorldRef> {
    using Owned::Owned;
};



//
// collision detection
//

class GeomRef;

namespace detail {
typedef std::function<void(Contact)> ContactsCallback;
typedef std::function<void(const GeomRef&, const GeomRef&)> CollideCallback;
void near_callback(void*, dxGeom*, dxGeom*);
void ray_near_callback(void*, dxGeom*, dxGeom*);
}

void contacts(const GeomRef&, const GeomRef&, detail::ContactsCallback);
void collide(const GeomRef&, const GeomRef&, detail::CollideCallback);

struct GeomRef : detail::Object_<dxGeom*> {
protected:
    using Object::Object;
    void destroy();
    void set_data(void*);
    static void* get_data(ID);

public:
    dloc location() const;
    void set_location(dloc);

    Body& body() const;

    friend class Mass;
    friend class BodyRef;
    friend class GeomBody;
    friend class RayRef;
    friend class SpaceRef;

    friend void contacts(const GeomRef&, const GeomRef&,
                         detail::ContactsCallback);
    friend void collide(const GeomRef&, const GeomRef&,
                        detail::CollideCallback);

    friend void ode::detail::near_callback(void*, dxGeom*, dxGeom*);
    friend void ode::detail::ray_near_callback(void*, dxGeom*, dxGeom*);

#ifndef NDEBUG
    friend ostream& operator<<(ostream&, const GeomRef&);
#endif
};

struct Geom : detail::Owned_<Geom, GeomRef> {
    // allow Geom to act as a kind of union for Box, TriMesh, etc.
    template<typename Shape, typename ShapeRef>
    Geom(detail::Owned_<Shape, ShapeRef>&& shape) :
        Owned(std::move(shape).release_id(), take_tag{}) {}

    using Owned::try_find;

    friend void ode::detail::near_callback(void*, dxGeom*, dxGeom*);
};


struct SphereRef : GeomRef {
protected:
    using GeomRef::GeomRef;
    ID create(double radius);

public:
    double radius() const;
    void set_radius(double);
};

struct Sphere : detail::Owned_<Sphere, SphereRef> {
    using Owned::Owned;
};

struct BoxRef : GeomRef {
protected:
    using GeomRef::GeomRef;
    ID create(dvec3 size);

public:
    dvec3 size() const;
};

struct Box : detail::Owned_<Box, BoxRef> {
    using Owned::Owned;
};

struct CapsuleRef : GeomRef {
protected:
    using GeomRef::GeomRef;
    ID create(double radius, double length);
};

struct Capsule : detail::Owned_<Capsule, CapsuleRef> {
    using Owned::Owned;
};


struct RayRef : GeomRef {
protected:
    using GeomRef::GeomRef;
    static ID create(double length);

public:
    struct Hit {
        dvec3 p;
        dvec3 normal;
        double distance;
        GeomRef geom;
    };

    optional<Hit> hit(const GeomRef&) const;
};

struct Ray : detail::Owned_<Ray, RayRef> {
    Ray(double length, dvec3 position, dvec3 direction);
    Ray(double length, dloc); // direction is local -z
};


struct TriMeshDataRef : detail::Object_<dxTriMeshData*> {
protected:
    using Object::Object;
    static ID create();
    void destroy();

    friend class TriMeshRef;
};

struct TriMeshData : detail::Owned_<TriMeshData, TriMeshDataRef> {
private:
    // must provide storage for these, ODE only keeps pointers
    unique_ptr<float[]> v;
    unique_ptr<int[]> f;

public:
    TriMeshData(const Mesh&);
};

struct TriMeshRef : GeomRef {
protected:
    using GeomRef::GeomRef;
    ID create(const TriMeshDataRef&);
};

struct TriMesh : detail::Owned_<TriMesh, TriMeshRef> {
    using Owned::Owned;
};


struct SpaceRef : GeomRef {
protected:
    using GeomRef::GeomRef;
    ID create();

public:
    SpaceRef& operator<<(const GeomRef& geom);

    Sphere new_sphere(double radius);
    Box new_box(dvec3 size);
    Capsule new_capsule(double radius, double length);
    TriMesh new_trimesh(const TriMeshData&);

    void collide(detail::CollideCallback);
};

struct Space : detail::Owned_<Space, SpaceRef> {
    using Owned::Owned;
};

}

#endif
