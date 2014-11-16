#include "preamble.glsl"
#include "common.glsl"

uniform mat4 M;

out vec2 t;

void main() {
    int i = gl_VertexID;
    t = vec2(i/2%2*2-1, i%2*2-1);
    vec4 position = vec4(0, t, 1);
    vec3 normal = vec3(1, 0, 0);
    float a = radians(180 * gl_InstanceID / 6.);
    mat4 R = mat4(mat2(cos(a), -sin(a), sin(a), cos(a)));
    gl_Position = P * V * M * R * (position * vec4(1, .2, 1, 1));
}
