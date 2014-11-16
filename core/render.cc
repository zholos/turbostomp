#include "render.h"

#include "assets.h"
#include "camera.h"
#include "debug.h"
#include "effect.h"
#include "grid.h"
#include "mesh.h"
#include "sea.h"

#include <glm/ext.hpp>

using std::vector;


Renderer::Renderer() :
    cube_prog(shaders["cube.vert"], shaders["cube.frag"]),
    tile_prog(shaders["tile.vert"], shaders["tile.frag"]),
    mesh_prog(shaders["mesh.vert"], shaders["mesh.frag"]),
    thruster_prog(shaders["thruster.vert"], shaders["thruster.frag"]),
    ball_prog(shaders["ball.vert"], shaders["ball.frag"]),
    bolt_prog(shaders["bolt.vert"], shaders["bolt.frag"]),
    cube_effect_prog(shaders["cube_effect.vert"], shaders["cube_effect.frag"]),
    post_prog(shaders["post.vert"], shaders["post.frag"])
{
#ifndef NDEBUG
    cube_prog.validate();
    mesh_prog.validate();
    thruster_prog.validate();
    ball_prog.validate();
    bolt_prog.validate();
    cube_effect_prog.validate();
    post_prog.validate();
#endif

    cube_prog.uniform_block("Common").bind(0);
    mesh_prog.uniform_block("Common").bind(0);
    thruster_prog.uniform_block("Common").bind(0);
    ball_prog.uniform_block("Common").bind(0);
    bolt_prog.uniform_block("Common").bind(0);
    cube_effect_prog.uniform_block("Common").bind(0);

    arrays.cube.positions.load(gl::cube_strip);

    Mesh(meshes["ramp.obj"]).triangles_with_wireframe(arrays.shape_pnb[0]);
    Mesh(meshes["corner1.obj"]).triangles_with_wireframe(arrays.shape_pnb[1]);
    Mesh(meshes["corner2.obj"]).triangles_with_wireframe(arrays.shape_pnb[2]);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_FRAMEBUFFER_SRGB);

    fbo.fbo.attach_color(fbo.color);
    fbo.fbo.attach_depth(fbo.depth);
    fbo.size = ivec2(-1); // resized on first render
}



//        Octree subdivision
//
//            ^
//           y|
//             2           3
//            +-----------+
//           /:    /     /|
//          /-----/-----/ |
//       6 /  :  /   7 /| |
//        +-----------+ |/|
//        |   :0|     | / |1   x
//        |   + |- - -|/| +   -->
//        |-----|-----| |/
//        | .   |     | /
//        |.    |     |/
//        +-----------+
//         4           5
//      /z
//     v
//
// - When viewed from this angle, only box 0 can be completely occluded.
// - The specific box that is occluded depends on the octant it is in relative
//   to the camera origin. Each octant is rendered separately to take advantage
//   of this.
// - More specifically here, box 0 is occluded by:
//    - the -x face of box 1,
//    - the -y face of box 2,
//    - the -z face of box 4.
// - Examining one level deeper:
//    - the -x face is composed of -x faces of boxes 0, 2, 4 and 6 (bit 0 = 0);
//    - the -y face is composed of -y faces of boxes 0, 1, 4 and 5 (bit 1 = 0);
//    - the -z face is composed of -z faces of boxes 0, 1, 2 and 3 (bit 2 = 0).

namespace {
struct OctantTileRenderer {
    VertexStream<glm::vec4, glm::vec3>& cube_stream;
    VertexStream<glm::vec4, glm::vec4, glm::vec3> (&shape_streams)[3];
    const Convex<6> frustum;
    Octant octant;

    ioct render(Grid::const_cursor cursor, bool all_inside = false) const {
        // assumes cursor is not null
        // TODO: limit depth for checks (don't bother with small boxes)
        bool cull = !all_inside;
        if (cull) {
            auto test =
                octant.box_test(cursor.box()) & frustum.box_test(cursor.box());
            if (test == ConvexTest::inside)
                all_inside = true;
            if (test != ConvexTest::outside)
                cull = false;
        }
        if (cull)
            // TODO: determine when a culled box occludes boxes behind it
            return ioct{0}; // might not occlude, e.g. culled by the near plane
        else if (!cursor.is_tile()) {
            // branch
            int occludes = 7;
            bool back_hidden = true;
            for (int i: { 7, 6, 5, 3, 4, 2, 1, 0 }) {
                if (i == 0 && back_hidden)
                    break;

                ioct j{i^octant.octant.i()};
                ioct o = render(cursor[j], all_inside);

                // occlusion derived from this box
                //if (!o.x() && !(i & 1))
                //    occludes &= 7 ^ 1;
                //...
                occludes &= o.i() | i;

                // occlusion of back subbox inside this box
                //if (!o.x() && i==1 || !o.y() && i==2 || !o.z() && i==4)
                if (!(i & i-1) && ~o.i() & i)
                    back_hidden = false;
            }
            return ioct{occludes};
        } else if (Tile t = cursor.tile()) {
            // non-empty tile
            // avoid double-painting by assigning each tile to one octant
            bool paint = octant.point_inside(cursor.box().p0());
            if (t.shape()) {
                auto l = t.shape_loc() + dvec3(cursor.box().p0());
                if (paint)
                    shape_streams[t.shape_mesh()-1].push(
                        glm::vec4(l.p, cursor.box().size()),
                        glm::vec4(l.q.x, l.q.y, l.q.z, l.q.w),
                        glm::vec3(t.color_vec4()));
                return ioct{0}; // not sure what this occludes
            } else {
                if (paint)
                    cube_stream.push(
                        glm::vec4(cursor.box().p0(), cursor.box().size()),
                        glm::vec3(t.color_vec4())); // TODO: pack color into int
                return ioct{7};
            }
        } else
            // empty tile
            return ioct{0};
    }
};
}

void
Renderer::render_tiles(const Projection& projection, const Grid& grid)
{
    size_t cubes_rendered = 0;
    vector<glm::vec4> ts;
    vector<glm::vec3> color;

    streams.cube.set_render([&] {
        WithProgram<0, 1> with(cube_prog);
        streams.cube.attribs_instanced(cube_prog, 0, 1);
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 8, streams.cube.size());
        cubes_rendered += streams.cube.size();
    });

    auto shape_render = [&] (int i) {
        // TODO: may need to correct grid on hypothetical large tiles
        WithProgram<0, 1, 2, 3, 4, 5> with(tile_prog);
        tile_prog.attrib(0).uninstanced().array(arrays.shape_pnb[i][0]);
        tile_prog.attrib(1).uninstanced().array(arrays.shape_pnb[i][1]);
        tile_prog.attrib(2).uninstanced().array(arrays.shape_pnb[i][2]);
        streams.shapes[i].attribs_instanced(tile_prog, 3, 4, 5);
        glDrawArraysInstanced(GL_TRIANGLES, 0, arrays.shape_pnb[i][0].size(),
                              streams.shapes[i].size());
    };

    for (int i = 0; i < 3; i++)
        streams.shapes[i].set_render([&,i] { shape_render(i); });

    for (auto q: ioct::all()) {
        auto R = glm::diagonal3x3(
            glm::mix(glm::vec3(1), glm::vec3(-1), q.bvec3_cast()));
        if (glm::determinant(R) < 0)
            // swap x and y axes to maintain orientation
            R = R * glm::mat3(glm::mat2(0, 1, 1, 0));

        cube_prog.use();
        cube_prog.uniform("R").set(glm::translate(glm::vec3(.5)) *
                                   glm::mat4(R) *
                                   glm::translate(glm::vec3(-.5)));
        OctantTileRenderer{streams.cube, streams.shapes,
                           projection.frustum(),
                           projection.octant(q)}.render(grid.ctop());
        streams.cube.flush();
    }

    for (auto& s: streams.shapes)
        s.flush();
}


struct SpriteStream::Data {
    map<const Mesh*, vector<glm::vec4>> mesh_locations;
    vector<glm::vec4> ball_locations, thruster_locations;
    vector<dloc> bolts;

    static void push(vector<glm::vec4>& v, dloc l, double scale = 1) {
        v.emplace_back(l.p.x, l.p.y, l.p.z, scale);
        v.emplace_back(l.q.x, l.q.y, l.q.z, l.q.w);
    }
};

void
SpriteStream::push_mesh(const Mesh* mesh, dloc l)
{
    Data::push(data.mesh_locations[mesh], l);
}

void
SpriteStream::push_bolt(dloc l)
{
    data.bolts.push_back(l);
}

void
SpriteStream::push_ball(dloc l, double radius)
{
    Data::push(data.ball_locations, l, radius);
}

void
SpriteStream::push_thruster(dloc l, bool hit, double distance)
{
    if (hit)
        Data::push(data.thruster_locations, l, distance);
}


void
Renderer::render_sprites(const Projection& projection, SpriteStream& stream)
{
    auto& data = stream.data;

    for (auto& ml: data.mesh_locations) {
        bool load = !meshes_data.count(ml.first);
        MeshData& mesh_data = meshes_data[ml.first];
        if (load)
            ml.first->triangles_with_wireframe(mesh_data.positions,
                                               mesh_data.normals,
                                               mesh_data.borders);

        gl::Array<GLfloat> locations{ml.second};
        WithProgram<0, 1, 2, 3, 4, 5> with(mesh_prog);
        mesh_prog.attrib(0).uninstanced().array(mesh_data.positions);
        mesh_prog.attrib(1).uninstanced().array(mesh_data.normals);
        mesh_prog.attrib(2).uninstanced().array(mesh_data.borders);
        mesh_prog.attrib(3).instanced().array(locations, 4, 0,
                                              8*sizeof(GLfloat));
        mesh_prog.attrib(4).instanced().array(locations, 4, 4,
                                              8*sizeof(GLfloat));
        mesh_prog.attrib(5).set(glm::vec4(0, .24, 1, 1));
        glDrawArraysInstanced(GL_TRIANGLES, 0, mesh_data.positions.size(),
                              ml.second.size()/2);
    }

    if (!data.ball_locations.empty()) {
        gl::Array<GLfloat> locations{data.ball_locations};
        WithProgram<0, 1, 2> with(ball_prog);
        ball_prog.attrib(0).uninstanced().array(arrays.cube.positions);
        ball_prog.attrib(1).instanced().array(locations,
                                              4, 0, 8*sizeof(GLfloat));
        ball_prog.attrib(2).instanced().array(locations,
                                              4, 4, 8*sizeof(GLfloat));
        glDrawArraysInstanced(GL_TRIANGLE_STRIP,
                              0, arrays.cube.positions.size(),
                              data.ball_locations.size()/2);
    }

    if (!data.thruster_locations.empty()) {
        gl::Array<GLfloat> locations{data.thruster_locations};
        WithProgram<0, 1, 2> with(thruster_prog);
        thruster_prog.attrib(0).uninstanced().array(arrays.cube.positions);
        thruster_prog.attrib(1).instanced().array(locations,
                                                  4, 0, 8*sizeof(GLfloat));
        thruster_prog.attrib(2).instanced().array(locations,
                                                  4, 4, 8*sizeof(GLfloat));
        glDrawArraysInstanced(GL_TRIANGLE_STRIP,
                              0, arrays.cube.positions.size(),
                              data.thruster_locations.size()/2);
    }
}


void
Renderer::render_bolts(const Projection& projection, SpriteStream& stream)
{
    glDepthMask(GL_FALSE);
    WithProgram<> with(bolt_prog);
    for (dloc& bolt: stream.data.bolts) {
        bolt_prog.uniform("M").set(glm::mat4(bolt.mat4_cast()));
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, 6);
    }
    glDepthMask(GL_TRUE);
}


void
Renderer::render_effects(const Projection& projection)
{
    WithProgram<0> with(cube_effect_prog);
    cube_effect_prog.attrib(0).uninstanced().array(arrays.cube.positions);
    for (auto effect: BoxEffect::all()) {
        cube_effect_prog.uniform("M").set(effect->b.mat4_cast());
        cube_effect_prog.uniform("i").set(effect->i);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, arrays.cube.positions.size());
    }
}


void
Renderer::render(ivec2 size, const Camera& camera,
                 const Grid& grid, const Sea& sea)
{
    background = dvec4(0, 0, .05, 1);

    Projection projection{camera, size};

    if (size != fbo.size) {
        fbo.color.reset_rgba(size);
        fbo.depth.reset_depth(size);
    }

    fbo.fbo.bind();

    {
        auto block = cube_prog.uniform_block("Common");
        common.clear(block.size_bytes());
        common.bind();
        auto P = projection.projection_matrix();
        auto V = projection.view_matrix();
        block.uniform("PV").set(glm::mat4(P * V));
        block.uniform("P").set(glm::mat4(P));
        block.uniform("V").set(glm::mat4(V));

        // mathematically V_origin.w is 1, but in GLSL this had severe accuracy
        // issues that were mitigated by the divide, so could be useful here too
        auto V_origin = glm::inverse(V) * glm::dvec4(0, 0, 0, 1);
        block.uniform("V_origin").set(glm::vec3(V_origin.xyz() / V_origin.w));

        common.unbind();
        common.bind(0);
    }

    glClearColor(background.r, background.g, background.b, background.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE); // TODO: check if supported
    glEnable(GL_CULL_FACE);

    render_tiles(projection, grid);

    SpriteStream::Data sprite_data;
    SpriteStream stream{sprite_data};
    sea.render(stream);

    render_sprites(projection, stream);

    fbo.fbo.unbind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_MULTISAMPLE);
    // no need to clear as we're writing the entire screen

    post_prog.use();
    fbo.color.bind(0);
    fbo.depth.bind(1);
    post_prog.uniform("background").set(glm::vec4(background));
    post_prog.uniform("color_buffer").set(0);
    post_prog.uniform("depth_buffer").set(1);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    post_prog.unuse();

    glDisable(GL_CULL_FACE);
    render_bolts(projection, stream);

    glEnable(GL_CULL_FACE);
    render_effects(projection);
}
