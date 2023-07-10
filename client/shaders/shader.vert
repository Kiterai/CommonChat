#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec3 inPos;

void main() {
    gl_Position = camera.proj * camera.view * vec4(inPos, 1.0);
}