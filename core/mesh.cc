#include "mesh.h"

#include <limits>
#include <sstream>

#include <glm/ext.hpp>

using std::getline;
using std::istringstream;
using std::numeric_limits;


Mesh::Mesh(string obj)
{
    istringstream lines(obj);
    string line_buf;
    while (getline(lines, line_buf)) {
        istringstream line(line_buf);
        string definition;
        line >> definition;
        if (definition == "v") {
            glm::vec3 u;
            line >> u.x >> u.y >> u.z;
            assert(line);
            v.emplace_back(u, 1);
        } else if (definition == "f") {
            glm::ivec4 u(0, 0, 0, -1);
            int i = 0;
            int n;
            while (line >> n) {
                assert(0 < n && n <= v.size());
                assert(i < 4);
                u[i++] = n-1;
            }
            assert(i >= 3);
            f.push_back(u);
        }
    }
}


void
Mesh::transform(glm::dmat4 M)
{
    for (glm::vec4& u: v)
        u = glm::vec4(M * glm::dvec4(u));
}


static float
altitude(glm::vec4 vertex, glm::vec4 base0, glm::vec4 base1)
{
    glm::vec4 side = base0 - vertex,
              base = base1 - base0;
    return glm::length(side - glm::proj(side, base));
}

void
Mesh::triangles_with_wireframe(gl::Array<glm::vec4>& positions_array,
                               gl::Array<glm::vec4>& normals_array,
                               gl::Array<glm::vec4>& borders_array) const
{
    vector<glm::vec4> positions, normals, borders;

    for (auto n: f)
        if (n.w < 0) {
            // triangle
            glm::vec3 normal = glm::normalize(glm::cross(
                glm::vec3(v[n[1]]-v[n[0]]),
                glm::vec3(v[n[2]]-v[n[1]])));
            for (int i = 0; i < 3; i++) {
                positions.push_back(v[n[i]]);
                normals.emplace_back(normal, 1);
                glm::vec4 b(0, 0, 0, numeric_limits<float>::infinity());
                b[i] = altitude(v[n[i]], v[n[(i+1)%3]], v[n[(i+2)%3]]);
                borders.push_back(b);
            }
        } else {
            // quad
            glm::vec3 normal(0);
            glm::vec4 b[4] = {};
            for (int i = 0; i < 4; i++) {
                normal += glm::cross(
                    glm::vec3(v[n[(i+1)%4]]-v[n[i]]),
                    glm::vec3(v[n[(i+3)%4]]-v[n[i]]));
                b[(i+2)%4][i] = altitude(v[n[(i+2)%4]], v[n[(i+1)%4]], v[n[i]]);
                b[(i+3)%4][i] = altitude(v[n[(i+3)%4]], v[n[i]], v[n[(i+1)%4]]);
            }
            normal = glm::normalize(normal);
            for (int i: { 0, 1, 2, 0, 2, 3 }) {
                positions.push_back(v[n[i]]);
                normals.emplace_back(normal, 1);
                borders.push_back(b[i]);
            }
        }

    positions_array.load(positions);
    normals_array.load(normals);
    borders_array.load(borders);
}


vector<glm::ivec3>
Mesh::tris() const
{
    vector<glm::ivec3> t;
    for (auto n: f)
        if (n.w < 0)
            t.emplace_back(n);
        else {
            t.emplace_back(n[0], n[1], n[2]);
            t.emplace_back(n[0], n[2], n[3]);
        }
    return t;
}
