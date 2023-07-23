#ifndef VULKAN_MODEL_MANAGER_HPP
#define VULKAN_MODEL_MANAGER_HPP

#include "Buffer.hpp"
#include "Image.hpp"
#include "Render.hpp"
#include <fastgltf/parser.hpp>
#include <filesystem>

class ModelManager {
    fastgltf::Parser gltfParser;

    vk::PhysicalDevice physDevice;
    vk::Device device;

    std::optional<ReadonlyBuffer> modelPosVertBuffer;
    std::optional<ReadonlyBuffer> modelNormVertBuffer;
    std::optional<ReadonlyBuffer> modelTexcoordVertBuffer;
    std::optional<ReadonlyBuffer> modelJointsVertBuffer;
    std::optional<ReadonlyBuffer> modelWeightsVertBuffer;
    std::optional<ReadonlyBuffer> modelIndexBuffer;
    std::vector<ReadonlyImage> textureAtlas;

public:
    struct MeshPointer {
        uint32_t vertexBase;
        uint32_t IndexBase;
        uint32_t indexNum;
    };

    struct ModelInfo {
        std::vector<MeshPointer> primitives;
        uint32_t jointNum;
    };

    ModelManager(vk::PhysicalDevice physDevice, vk::Device device);
    MeshPointer allocate(uint32_t vertNum, uint32_t indNum);
    void prepareRender(RenderDetails &rd);
    ModelInfo loadModelFromGlbFile(const std::filesystem::path path, vk::Queue queue, vk::CommandBuffer cmdBuf, vk::Fence fence);
};

#endif VULKAN_MODEL_MANAGER_HPP
