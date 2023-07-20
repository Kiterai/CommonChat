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

struct ObjectData{
	mat4 model;
    mat4 joints[32];
};

layout(set = 0, binding = 2) readonly buffer ObjectBuffer{
	ObjectData objects[];
} objectBuffer;

void main() {
    mat4 skinMat = 
        inWeight.x * objectBuffer.objects[gl_InstanceIndex].joints[inJoints.x] +
        inWeight.y * objectBuffer.objects[gl_InstanceIndex].joints[inJoints.y] +
        inWeight.z * objectBuffer.objects[gl_InstanceIndex].joints[inJoints.z] +
        inWeight.w * objectBuffer.objects[gl_InstanceIndex].joints[inJoints.w];

    vec4 worldPos = objectBuffer.objects[gl_InstanceIndex].model * skinMat * vec4(inPos, 1.0);
    gl_Position = camera.proj * camera.view * worldPos;
}