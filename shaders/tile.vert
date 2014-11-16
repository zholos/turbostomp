#include "preamble.glsl"
#include "lib.glsl"
#include "common.glsl"

layout(location=0) in vec4 position;
layout(location=1) in vec4 normal;
layout(location=2) in vec4 border_;
layout(location=3) in vec4 l_ps;
layout(location=4) in vec4 l_q;
layout(location=5) in vec4 color;

flat out vec4 face_color, edge_color;
out vec4 border;
out vec2 t;

void main() {
    // TODO: check border color on lower side
    float light = -log(.5+.5*tile_light_factor*
        dot(vec3(normal), quat_rotate(quat_conjugate(l_q), light_source)));
    face_color = adjust_luma(color, light);
    edge_color = adjust_luma(face_color, .75);

    border = border_;

    // for fwidth(), like in cube
    // because some faces are not axis-aligned, we pick two axes in the face
    // (perpendicular to the normal) and project position onto them
    vec3 n = normal.xyz,
         nx = cross(n, normalize(n.zxy+vec3(1,0,0))), ny = cross(n, nx);
    t = vec2(dot(position.xyz, nx), dot(position.xyz, ny));

    gl_Position = P * V * loc_apply(loc(l_ps, l_q), position);
}
