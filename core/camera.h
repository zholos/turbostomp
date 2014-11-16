#ifndef CORE_CAMERA_H
#define CORE_CAMERA_H

#include "box.h"
#include "octant.h"

#include <pgamecc.h>

using pgamecc::ivec2;
using pgamecc::dvec3;
using pgamecc::dloc;


struct Camera {
    dloc l;
    double fov = glm::radians(60.); // vertical radians

    void track(dvec3 eye, dvec3 watch, dvec3 up, double smoothing = 0);
};


class Projection {
public: // TODO: private
    const Camera& camera;
    ivec2 _size;

public:
    Projection(const Camera& camera, ivec2 size) :
        camera(camera), _size(size) {}

    glm::dmat4 view_matrix() const;
    glm::dmat4 projection_matrix() const;
    glm::dmat4 overlay_matrix() const; // origin at bottom-left
    ivec2 size() const { return _size; }

    Convex<6> frustum() const;
    Octant octant(ioct) const;
};


#endif
