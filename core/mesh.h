#ifndef CORE_MESH_H
#define CORE_MESH_H

#include <pgamecc.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

namespace gl = pgamecc::gl;


class Mesh {
    vector<glm::vec4> v;
    vector<glm::ivec4> f;

public:
    Mesh(string obj);
    Mesh(string obj, glm::dmat4 M) : Mesh(obj) { transform(M); }
    Mesh(pgamecc::Source obj) : Mesh(obj.source) {}
    Mesh(pgamecc::Source obj, glm::dmat4 M) : Mesh(obj.source, M) {}
    void transform(glm::dmat4);

    void triangles_with_wireframe(gl::Array<glm::vec4>& positions,
                                  gl::Array<glm::vec4>& normals,
                                  gl::Array<glm::vec4>& borders) const;
    void triangles_with_wireframe(gl::Array<glm::vec4> pnb[3]) const {
        triangles_with_wireframe(pnb[0], pnb[1], pnb[2]);
    }

    const vector<glm::vec4>& verts() const { return v; }
    vector<glm::ivec3> tris() const;
};


#endif
