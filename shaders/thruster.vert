#include "preamble.glsl"
#include "lib.glsl"
#include "common.glsl"

layout(location=0) in vec4 position;
layout(location=1) in vec4 l_ps;
layout(location=2) in vec4 l_q;

out vec3 t;

const float radius = .1;

void main() {
    // box of fixed radius pointing to parametric distance (l.ps.w) into -z
    vec3 p = position.xyz * vec3(2*radius, 2*radius, l_ps.w) -
                            vec3(radius, radius, l_ps.w);
    t = position.xyz;

    gl_Position = PV * loc_apply(loc(vec4(l_ps.xyz, 1), l_q), vec4(p, 1));
}
