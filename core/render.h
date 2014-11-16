#ifndef CORE_RENDER_H
#define CORE_RENDER_H

#include "glext.h"

#include <map>

#include <pgamecc.h>

using std::map;
using pgamecc::ivec2;
using pgamecc::dvec4;
using pgamecc::dloc;
namespace gl = pgamecc::gl;

class Projection;
class Camera;
class Grid;
class Sea;
class Mesh;


struct SpriteStream {
    struct Data;
    Data& data;

    // Mesh must persist, so arrays can be created in render thread, not in
    // sprite data gathering thread
    void push_mesh(const Mesh*, dloc);

    void push_bolt(dloc);
    void push_ball(dloc, double radius);
    void push_thruster(dloc, bool hit, double distance);
};


class Renderer {
    gl::Program cube_prog, tile_prog, mesh_prog, thruster_prog, ball_prog,
                bolt_prog, cube_effect_prog, post_prog;
    gl::UniformBuffer<char> common;
    struct {
        struct {
            gl::Array<glm::vec4> positions;
        } cube;
        gl::Array<glm::vec4> shape_pnb[3][3];
    } arrays;

    struct MeshData {
        gl::Array<glm::vec4> positions, normals, borders;
    };
    map<const Mesh*, MeshData> meshes_data;

    struct {
        VertexStream<glm::vec4, glm::vec3> cube;
        VertexStream<glm::vec4, glm::vec4, glm::vec3> shapes[3];
    } streams;
    struct {
        gl::Texture color, depth;
        gl::Framebuffer fbo;
        ivec2 size;
    } fbo;

    dvec4 background;

    void render_tiles(const Projection&, const Grid&);
    void render_sprites(const Projection&, SpriteStream&);
    void render_bolts(const Projection&, SpriteStream&);
    void render_effects(const Projection&);

public:
    Renderer();
    void render(ivec2 size, const Camera&, const Grid&, const Sea&);
};


#endif
