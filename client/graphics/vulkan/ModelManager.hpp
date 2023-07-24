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
    vk::UniqueDescriptorSetLayout modelDescSetLayout;
    vk::UniqueDescriptorSet modelDescSet;

    std::optional<ReadonlyBuffer> modelPosVertBuffer;
    std::optional<ReadonlyBuffer> modelNormVertBuffer;
    std::optional<ReadonlyBuffer> modelTexcoordVertBuffer;
    std::optional<ReadonlyBuffer> modelJointsVertBuffer;
    std::optional<ReadonlyBuffer> modelWeightsVertBuffer;
    std::optional<ReadonlyBuffer> modelIndexBuffer;
    std::vector<ReadonlyImage> textureAtlas;
    std::vector<vk::UniqueImageView> textureImageViews;

    std::optional<ReadonlyBuffer> modelInfoBuffer;
    std::optional<ReadonlyBuffer> primitiveInfoBuffer;
    std::optional<ReadonlyBuffer> materialInfoBuffer;
    std::optional<ReadonlyBuffer> jointsInfoBuffer;

    std::optional<ReadonlyImage> defaultTexture;
    vk::UniqueImageView defaultTextureImgView;
    vk::UniqueSampler defaultSampler;

  public:
    struct MeshPointer {
        uint32_t vertexBase;
        uint32_t IndexBase;
        uint32_t indexNum;
        uint32_t materialIndex;
        uint32_t textureIndex; // no longer used
    };

    struct ModelInfo {
        std::vector<MeshPointer> primitives;
        uint32_t nodeNum;
    };

    ModelManager(vk::PhysicalDevice physDevice, vk::Device device, vk::DescriptorPool pool, vk::Queue queue, vk::CommandBuffer cmdBuf, vk::Fence fence);
    MeshPointer allocate(uint32_t vertNum, uint32_t indNum);
    void prepareRender(RenderDetails &rd);
    ModelInfo loadModelFromGlbFile(const std::filesystem::path path, vk::Queue queue, vk::CommandBuffer cmdBuf, vk::Fence fence);
    const auto &getTextureImageViews() const { return textureImageViews; } // no longer used;
};

#endif VULKAN_MODEL_MANAGER_HPP
