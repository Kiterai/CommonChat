#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
} camera;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inTexcoord;
layout(location = 3) in uvec4 inJoints;
layout(location = 4) in vec4 inWeight;

mat4 u_jointMat[2] = {
    mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    ),
    mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(0.0, 1.0, 0.0, 1.0)
    ) * 
    mat4(
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(-1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(0.0, 0.0, 0.0, 1.0)
    ) * 
    mat4(
        vec4(1.0, 0.0, 0.0, 0.0),
        vec4(0.0, 1.0, 0.0, 0.0),
        vec4(0.0, 0.0, 1.0, 0.0),
        vec4(0.0,-1.0, 0.0, 1.0)
    )
};

void main() {
    mat4 skinMat = 
        inWeight.x * u_jointMat[inJoints.x] +
        inWeight.y * u_jointMat[inJoints.y] +
        inWeight.z * u_jointMat[inJoints.z] +
        inWeight.w * u_jointMat[inJoints.w];

    vec4 worldPos = skinMat * vec4(inPos, 1.0);
    gl_Position = camera.proj * camera.view * worldPos;
}