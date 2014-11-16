#include "preamble.glsl"
#include "lib.glsl"
#include "common.glsl"

uniform mat4 R; // rotate symmetrical half-box to face viewport
layout(location=0) in vec4 ts; // translate, size
layout(location=1) in vec4 color;

flat out vec4 face_color, edge_color;
out vec2 t;

void main() {
    // Use 8 verts to draw a triangle fan for 3 faces centered on (1, 1, 1).
    // Each face has two tris. The normal for both is:
    int i = gl_VertexID-1;
    vec4 normal = vec4(equal(ivec3(i/2%3), ivec3(0, 1, 2)), 1);
    // The first provoking vertex matches the normal. The second can be
    // reached with a swizzle, e.g. (1, 0, 0) -> (1, 1, 0).
    vec4 position = i<0 ? vec4(1) : normal + i%2*vec4(normal.zxy, 0);

    // Grid texture can be viewed as a 3D texture, but is handled more simply
    // as a 2D texture mapped to faces.
    t = ts.w * (i<0 ? vec2(0) : vec2(1, 1-i%2));

    float light = -log(.5+.5*tile_light_factor*dot(
        vec3(transpose(inverse(R))*normal), light_source));
    face_color = adjust_luma(color, light);
    edge_color = adjust_luma(face_color, .75);

    mat4 M = mat4(ts.w, 0, 0, 0,
                  0, ts.w, 0, 0,
                  0, 0, ts.w, 0,
                  ts.xyz, 1);
    gl_Position = P * V * M * R * position;
}
