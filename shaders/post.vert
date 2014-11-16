#include "preamble.glsl"

out vec2 t;

void main() {
    int i = gl_VertexID;
    t = vec2(i%2, i/2%2);
    gl_Position = vec4(t*2-1, 0, 1);
}
