#include "preamble.glsl"
#include "common.glsl"

uniform mat4 M;
uniform int i;
layout(location=0) in vec4 position;
flat out vec4 color;

void main() {
    float level = sin(radians(180*(i+1)/20.));
    color = vec4(1, 1, 1, .2*level);
    gl_Position = P * V * M * vec4(((position.xyz-.5)*1.01+.5), 1);
}
