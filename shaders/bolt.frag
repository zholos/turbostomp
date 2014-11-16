#include "preamble.glsl"

in vec2 t;

out vec4 fragColor;

void main() {
    fragColor = vec4(1, 0, 0, (1-length(t)) / 2);
}
