#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 2) in vec2 inTexcoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inTexcoord.x, inTexcoord.y, 0.0, 1.0);
}