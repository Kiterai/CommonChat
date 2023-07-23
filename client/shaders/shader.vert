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

layout(location = 2) out vec2 outTexcoord;

struct ObjectData{
	mat4 model;
    uint jointIndex;
    uint dummy[3];
};

struct MeshData{
    uint objectIndex;
    uint materialIndex;
    uint textureIndex;
    uint dummy[1];
};

layout(set = 0, binding = 2) readonly buffer ObjectBuffer{
	ObjectData objects[];
} objectBuffer;

layout(set = 0, binding = 3) readonly buffer JointBuffer{
	mat4 joints[];
} jointBuffer;

layout(set = 0, binding = 4) readonly buffer MeshBuffer{
	MeshData meshes[];
} meshBuffer;

void main() {
    uint objectIndex = meshBuffer.meshes[gl_InstanceIndex].objectIndex;
    uint jointIndex = objectBuffer.objects[objectIndex].jointIndex;
    mat4 skinMat = 
        inWeight.x * jointBuffer.joints[jointIndex + inJoints.x] +
        inWeight.y * jointBuffer.joints[jointIndex + inJoints.y] +
        inWeight.z * jointBuffer.joints[jointIndex + inJoints.z] +
        inWeight.w * jointBuffer.joints[jointIndex + inJoints.w];

    vec4 worldPos = objectBuffer.objects[objectIndex].model * skinMat * vec4(inPos, 1.0);
    gl_Position = camera.proj * camera.view * worldPos;

    outTexcoord = inTexcoord;
}