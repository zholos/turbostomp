#include "preamble.glsl"
#include "lib.glsl"
#include "common.glsl"

layout(location=0) in vec4 position;
layout(location=1) in vec4 l_ps;
layout(location=2) in vec4 l_q;

flat out vec4 face_color, edge_color;
flat out vec4 Sr2; // S, r^2
out vec3 wb;
flat out vec4 q;

void main() {
    face_color = vec4(.45, 0, 0, 1);
    edge_color = adjust_luma(face_color, .75);

    Sr2 = vec4(l_ps.xyz - V_origin, l_ps.w*l_ps.w);
    wb = position.xyz*2-1;
    q = quat_conjugate(l_q);
    gl_Position = PV * vec4(l_ps.xyz + wb, 1);
}
