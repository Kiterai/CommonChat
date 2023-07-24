#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 1, binding = 3) uniform sampler2D texSampler[];

layout(location = 2) in vec2 inTexcoord;
layout(location = 4) flat in uint inTexIndex;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler[inTexIndex], inTexcoord);
}