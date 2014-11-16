#include "preamble.glsl"
#include "lib.glsl"

flat in vec4 face_color, edge_color;
in vec4 border;
in vec2 t;

out vec4 fragColor;

void main() {
    float f = comp_min(border) * pow(fwidth(t.x+t.y), -.25);
    fragColor = mix(edge_color, face_color,
        smoothstep(.08, .09, f) + 1-smoothstep(.01, .02, f));
}
