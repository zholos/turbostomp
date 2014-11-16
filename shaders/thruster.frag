#include "preamble.glsl"

in vec3 t;
out vec4 fragColor;

void main() {
    fragColor = vec4(.5, 0, 1, .5 + .1 * floor(3*t.z));
}
