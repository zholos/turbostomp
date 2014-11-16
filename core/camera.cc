#include "camera.h"

#include <glm/ext.hpp>


//
// Camera
//

void
Camera::track(dvec3 eye, dvec3 watch, dvec3 up, double smoothing)
{
    if (!smoothing)
        l.p = eye; // l.p may not be set yet
    else
        l.p += (1 - smoothing) * (eye - l.p);

    l = ~dloc::from_mat4(glm::lookAt(l.p, watch, up));
}


//
// Projection
//

glm::dmat4
Projection::view_matrix() const
{
    return (~camera.l).mat4_cast();
}

glm::dmat4
Projection::projection_matrix() const
{
    return glm::perspective(camera.fov, (double)_size.x / _size.y, 1., 200.);
}

glm::dmat4
Projection::overlay_matrix() const
{
    return glm::ortho(0., 0.+_size.x, 0., 0.+_size.y);
}


static dvec4
normalize_plane(dvec4 plane)
{
    return plane / glm::length(dvec3(plane));
}

Convex<6>
Projection::frustum() const
{
    auto PV = projection_matrix() * view_matrix();

    // plane' = plane * M^-1
    return {
        normalize_plane(dvec4(-1, 0, 0, 1) * PV),
        normalize_plane(dvec4( 1, 0, 0, 1) * PV),
        normalize_plane(dvec4(0, -1, 0, 1) * PV),
        normalize_plane(dvec4(0,  1, 0, 1) * PV),
        normalize_plane(dvec4(0, 0, -1, 1) * PV),
        normalize_plane(dvec4(0, 0,  1, 1) * PV),
    };
}


Octant
Projection::octant(ioct i) const
{
    return { ivec3(camera.l.p), i };
}
