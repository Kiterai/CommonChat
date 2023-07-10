#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SceneData {
    vec3 v;
} sceneData;

layout(location = 0) in vec3 inPos;

void main() {
    gl_Position = vec4(inPos + sceneData.v, 1.0);
}