#include "Modelmanager.hpp"
#include "Buffer.hpp"
#include "Helper.hpp"
#include "Image.hpp"
#include "Render.hpp"
#include <glm/glm.hpp>
#include <stb_image.h>

constexpr uint32_t maxVertNum = 1048576;
constexpr uint32_t maxIndNum = 4194304;
constexpr uint32_t maxTexNum = 32;
constexpr uint32_t maxModelNum = 1024;
constexpr uint32_t maxPrimitiveNum = 32768;
constexpr uint32_t maxMaterialNum = 32768;
constexpr uint32_t maxJointNum = 65536;

struct JointInfo {
    glm::mat4 inverseBindMatrix;
    uint32_t nodeIndex;
};

struct MaterialInfo {
    uint32_t colorTextureIndex;
};

struct PrimitiveInfo {
    uint32_t materialIndex;
    uint32_t morphInfoNum;
    uint32_t morphInfoBaseIndex;
    uint32_t jointInfoBaseIndex;
};

struct ModelInfoForShader {
    uint32_t primitiveNum;
    uint32_t primitiveInfoBaseIndex;
};

namespace {

vk::DescriptorSetLayoutBinding buildDescSetLayoutBinding(uint32_t bindingNum, vk::DescriptorType type, uint32_t count, vk::ShaderStageFlags stage) {
    vk::DescriptorSetLayoutBinding binding;
    binding.binding = bindingNum;
    binding.descriptorType = type;
    binding.descriptorCount = count;
    binding.stageFlags = stage;
    return binding;
}

vk::UniqueDescriptorSetLayout createDescLayout(vk::Device device) {
    std::vector<vk::DescriptorSetLayoutBinding> binding;
    // Model
    binding.push_back(buildDescSetLayoutBinding(
        0,
        vk::DescriptorType::eStorageBuffer,
        1,
        vk::ShaderStageFlagBits::eCompute));
    // Primitives Info
    binding.push_back(buildDescSetLayoutBinding(
        1,
        vk::DescriptorType::eStorageBuffer,
        1,
        vk::ShaderStageFlagBits::eFragment));
    // Material
    binding.push_back(buildDescSetLayoutBinding(
        2,
        vk::DescriptorType::eStorageBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex));
    // Texture
    binding.push_back(buildDescSetLayoutBinding(
        3,
        vk::DescriptorType::eCombinedImageSampler,
        maxTexNum,
        vk::ShaderStageFlagBits::eFragment));
    // Joints Info
    binding.push_back(buildDescSetLayoutBinding(
        4,
        vk::DescriptorType::eStorageBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex));
    // // Morph Targets (POSITION)
    // binding.push_back(buildDescSetLayoutBinding(
    //     5,
    //     vk::DescriptorType::eStorageBuffer,
    //     1,
    //     vk::ShaderStageFlagBits::eVertex));
    // // Morph Targets (NORMAL)
    // binding.push_back(buildDescSetLayoutBinding(
    //     6,
    //     vk::DescriptorType::eStorageBuffer,
    //     1,
    //     vk::ShaderStageFlagBits::eVertex));

    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.bindingCount = std::size(binding);
    createInfo.pBindings = binding.data();

    return device.createDescriptorSetLayoutUnique(createInfo);
}

std::vector<vk::UniqueDescriptorSet> createDescSets(vk::Device device, vk::DescriptorPool pool, vk::DescriptorSetLayout layout, uint32_t n) {
    std::vector<vk::DescriptorSetLayout> layouts(n, layout);
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = pool;
    allocInfo.pSetLayouts = layouts.data();
    allocInfo.descriptorSetCount = layouts.size();
    return device.allocateDescriptorSetsUnique(allocInfo);
}

vk::UniqueSampler createSampler(vk::Device device) {
    vk::SamplerCreateInfo createInfo;
    createInfo.magFilter = vk::Filter::eLinear;
    createInfo.minFilter = vk::Filter::eLinear;
    createInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    createInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    createInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    createInfo.anisotropyEnable = false;
    createInfo.maxAnisotropy = 1.0f;
    createInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    createInfo.unnormalizedCoordinates = false;
    createInfo.compareEnable = false;
    createInfo.compareOp = vk::CompareOp::eAlways;
    createInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    createInfo.mipLodBias = 0.0f;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = 0.0f;

    return device.createSamplerUnique(createInfo);
}

} // namespace

ModelManager::ModelManager(vk::PhysicalDevice physDevice, vk::Device device, vk::DescriptorPool pool, vk::Queue queue, vk::CommandBuffer cmdBuf, vk::Fence fence) : physDevice{physDevice}, device{device} {
    modelPosVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::vec3) * maxVertNum);
    modelNormVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::vec3) * maxVertNum);
    modelTexcoordVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::vec2) * maxVertNum);
    modelJointsVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::u16vec4) * maxVertNum);
    modelWeightsVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::vec4) * maxVertNum);
    modelIndexBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eIndexBuffer, sizeof(uint32_t) * maxIndNum);

    modelInfoBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eStorageBuffer, sizeof(ModelInfoForShader) * maxModelNum);
    primitiveInfoBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eStorageBuffer, sizeof(PrimitiveInfo) * maxPrimitiveNum);
    materialInfoBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eStorageBuffer, sizeof(MaterialInfo) * maxMaterialNum);
    jointsInfoBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eStorageBuffer, sizeof(JointInfo) * maxJointNum);

    {
        int texWidth, texHeight, texChannels;
        auto pixels = stbi_load("texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        defaultTexture.emplace(physDevice, device, queue, cmdBuf, pixels,
                               vk::Extent3D{static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1}, 1,
                               vk::ImageUsageFlagBits::eSampled, fence);

        stbi_image_free(pixels);
    }
    defaultTextureImgView = createImageViewFromImage(device, defaultTexture->getImage(), vk::Format::eR8G8B8A8Srgb, 1);
    defaultSampler = createSampler(device);

    modelDescSetLayout = createDescLayout(device);
    modelDescSet = std::move(createDescSets(device, pool, modelDescSetLayout.get(), 1)[0]);

    {
        vk::DescriptorBufferInfo modelBufDesc;
        modelBufDesc.buffer = modelInfoBuffer->getBuffer();
        modelBufDesc.offset = 0;
        modelBufDesc.range = sizeof(ModelInfoForShader) * maxModelNum;
        vk::DescriptorBufferInfo primitiveBufDesc;
        primitiveBufDesc.buffer = primitiveInfoBuffer->getBuffer();
        primitiveBufDesc.offset = 0;
        primitiveBufDesc.range = sizeof(PrimitiveInfo) * maxPrimitiveNum;
        vk::DescriptorBufferInfo materialBufDesc;
        materialBufDesc.buffer = materialInfoBuffer->getBuffer();
        materialBufDesc.offset = 0;
        materialBufDesc.range = sizeof(MaterialInfo) * maxMaterialNum;
        vk::DescriptorImageInfo textureDesc[maxTexNum];
        for (uint32_t i = 0; i < maxTexNum; i++) {
            textureDesc[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            textureDesc[i].imageView = defaultTextureImgView.get();
            textureDesc[i].sampler = defaultSampler.get();
        }
        vk::DescriptorBufferInfo jointsBufDesc;
        jointsBufDesc.buffer = jointsInfoBuffer->getBuffer();
        jointsBufDesc.offset = 0;
        jointsBufDesc.range = sizeof(JointInfo) * maxJointNum;

        vk::WriteDescriptorSet writeDescSet[5];
        writeDescSet[0].dstSet = modelDescSet.get();
        writeDescSet[0].dstBinding = 0;
        writeDescSet[0].dstArrayElement = 0;
        writeDescSet[0].descriptorCount = 1;
        writeDescSet[0].descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescSet[0].pBufferInfo = &modelBufDesc;
        writeDescSet[1].dstSet = modelDescSet.get();
        writeDescSet[1].dstBinding = 1;
        writeDescSet[1].dstArrayElement = 0;
        writeDescSet[1].descriptorCount = 1;
        writeDescSet[1].descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescSet[1].pBufferInfo = &primitiveBufDesc;
        writeDescSet[2].dstSet = modelDescSet.get();
        writeDescSet[2].dstBinding = 2;
        writeDescSet[2].dstArrayElement = 0;
        writeDescSet[2].descriptorCount = 1;
        writeDescSet[2].descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescSet[2].pBufferInfo = &materialBufDesc;
        writeDescSet[3].dstSet = modelDescSet.get();
        writeDescSet[3].dstBinding = 3;
        writeDescSet[3].dstArrayElement = 0;
        writeDescSet[3].descriptorCount = maxTexNum;
        writeDescSet[3].descriptorType = vk::DescriptorType::eCombinedImageSampler;
        writeDescSet[3].pImageInfo = textureDesc;
        writeDescSet[4].dstSet = modelDescSet.get();
        writeDescSet[4].dstBinding = 4;
        writeDescSet[4].dstArrayElement = 0;
        writeDescSet[4].descriptorCount = 1;
        writeDescSet[4].descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescSet[4].pBufferInfo = &jointsBufDesc;
        device.updateDescriptorSets(writeDescSet, {});
    }
}

ModelManager::MeshPointer ModelManager::allocate(uint32_t vertNum, uint32_t indNum) {
    return MeshPointer{0, 0, 0, 0, 0};
}

void ModelManager::prepareRender(RenderDetails &rd) {
    rd.positionVertBuf = modelPosVertBuffer.value().getBuffer();
    rd.normalVertBuf = modelNormVertBuffer.value().getBuffer();
    rd.texcoordVertBuf[0] = modelTexcoordVertBuffer.value().getBuffer();
    rd.jointsVertBuf[0] = modelJointsVertBuffer.value().getBuffer();
    rd.weightsVertBuf[0] = modelWeightsVertBuffer.value().getBuffer();
    rd.indexBuf = modelIndexBuffer.value().getBuffer();
    rd.assetDescSet = modelDescSet.get();
}

ModelManager::ModelInfo ModelManager::loadModelFromGlbFile(const std::filesystem::path path, vk::Queue queue, vk::CommandBuffer cmdBuf, vk::Fence fence) {
    fastgltf::GltfDataBuffer buffer;
    buffer.loadFromFile(path);
    auto gltf = gltfParser.loadBinaryGLTF(&buffer, path.parent_path());
    gltf->parse();
    if (auto err = gltf->validate(); err != fastgltf::Error::None)
        throw std::runtime_error("error on load glb");
    auto asset = gltf->getParsedAsset();

    auto datToSpan = [&](fastgltf::DataSource dat) {
        auto bufferViewIndex = std::get<fastgltf::sources::BufferView>(dat).bufferViewIndex;
        const auto &bufferView = asset->bufferViews[bufferViewIndex];
        const auto &bufferBytes = std::get<fastgltf::sources::ByteView>(asset->buffers[bufferView.bufferIndex].data).bytes;
        return fastgltf::span<const std::byte>(bufferBytes.data() + bufferView.byteOffset, bufferView.byteLength);
    };
    auto accessorToSpan = [&](size_t index) {
        const auto &accessor = asset->accessors[index];
        const auto &bufferView = asset->bufferViews[accessor.bufferViewIndex.value()];
        const auto &bufferBytes = std::get<fastgltf::sources::ByteView>(asset->buffers[bufferView.bufferIndex].data).bytes;
        return fastgltf::span<const std::byte>(bufferBytes.data() + bufferView.byteOffset, bufferView.byteLength);
    };

    uint32_t vertNumSum = 0, indNumSum = 0;
    for (const auto &mesh : asset->meshes) {
        for (const auto &primitive : mesh.primitives) {
            if (primitive.type != fastgltf::PrimitiveType::Triangles)
                throw std::runtime_error("Primitive type not supported");
            for (const auto &[attrName, attrIndex] : primitive.attributes) {
                vertNumSum += asset->accessors[primitive.attributes.begin()->second].count;
                indNumSum += asset->accessors[primitive.indicesAccessor.value()].count;
            }
        }
    }

    ModelInfo info;
    info.nodeNum = asset->nodes.size();

    MeshPointer pPrimitiveBase = allocate(vertNumSum, indNumSum);

    for (const auto &image : asset->images) {
        const auto imageData = datToSpan(image.data);

        int w, h, ch;
        auto pImage = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(imageData.data()), imageData.size_bytes(), &w, &h, &ch, STBI_rgb_alpha);
        if (!pImage)
            throw std::runtime_error("failed to load texture image");

        textureAtlas.emplace_back(physDevice, device, queue, cmdBuf, pImage, vk::Extent3D{uint32_t(w), uint32_t(h), 1}, 1,
                                  vk::ImageUsageFlagBits::eSampled, fence);
        stbi_image_free(pImage);
    }
    std::vector<vk::DescriptorImageInfo> textureDesc(textureAtlas.size());
    for (uint32_t i = 0; i < textureAtlas.size(); i++) {
        textureImageViews.emplace_back(createImageViewFromImage(device, textureAtlas[i].getImage(), vk::Format::eR8G8B8A8Srgb, 1));
        textureDesc[i].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        textureDesc[i].imageView = textureImageViews.back().get();
        textureDesc[i].sampler = defaultSampler.get();
    }
    vk::WriteDescriptorSet writeDescSet;
    writeDescSet.dstSet = modelDescSet.get();
    writeDescSet.dstBinding = 3;
    writeDescSet.dstArrayElement = 0;
    writeDescSet.descriptorCount = textureDesc.size();
    writeDescSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;
    writeDescSet.pImageInfo = textureDesc.data();
    device.updateDescriptorSets({writeDescSet}, {});

    MeshPointer pCurrentPrimitive = pPrimitiveBase;
    for (const auto &mesh : asset->meshes) {
        for (const auto &primitive : mesh.primitives) {
            for (const auto &[attrName, attrAccessorIndex] : primitive.attributes) {
                const auto attrData = accessorToSpan(attrAccessorIndex);
                if (attrName == "POSITION") {
                    modelPosVertBuffer->write(physDevice, device, queue, cmdBuf,
                                              static_cast<const void *>(attrData.data()), attrData.size_bytes(),
                                              pCurrentPrimitive.vertexBase * sizeof(glm::vec3), fence);
                } else if (attrName == "NORMAL") {
                    modelNormVertBuffer->write(physDevice, device, queue, cmdBuf,
                                               static_cast<const void *>(attrData.data()), attrData.size_bytes(),
                                               pCurrentPrimitive.vertexBase * sizeof(glm::vec3), fence);
                } else if (attrName == "TEXCOORD_0") {
                    modelTexcoordVertBuffer->write(physDevice, device, queue, cmdBuf,
                                                   static_cast<const void *>(attrData.data()), attrData.size_bytes(),
                                                   pCurrentPrimitive.vertexBase * sizeof(glm::vec2), fence);
                } else if (attrName == "JOINTS_0") {
                    modelJointsVertBuffer->write(physDevice, device, queue, cmdBuf,
                                                 static_cast<const void *>(attrData.data()), attrData.size_bytes(),
                                                 pCurrentPrimitive.vertexBase * sizeof(glm::u16vec4), fence);
                } else if (attrName == "WEIGHTS_0") {
                    modelWeightsVertBuffer->write(physDevice, device, queue, cmdBuf,
                                                  static_cast<const void *>(attrData.data()), attrData.size_bytes(),
                                                  pCurrentPrimitive.vertexBase * sizeof(glm::vec4), fence);
                }
            }
            {
                const auto indexData = accessorToSpan(primitive.indicesAccessor.value());
                modelIndexBuffer->write(physDevice, device, queue, cmdBuf,
                                        static_cast<const void *>(indexData.data()), indexData.size_bytes(),
                                        pCurrentPrimitive.IndexBase * sizeof(uint32_t), fence);
            }

            pCurrentPrimitive.indexNum = asset->accessors[primitive.indicesAccessor.value()].count;
            pCurrentPrimitive.materialIndex = pPrimitiveBase.materialIndex + primitive.materialIndex.value();
            pCurrentPrimitive.textureIndex = pPrimitiveBase.textureIndex +
                                             asset->textures[asset->materials[primitive.materialIndex.value()].pbrData->baseColorTexture->textureIndex].imageIndex.value();
            info.primitives.push_back(pCurrentPrimitive);

            pCurrentPrimitive.vertexBase += asset->accessors[primitive.attributes.begin()->second].count;
            pCurrentPrimitive.IndexBase += asset->accessors[primitive.indicesAccessor.value()].count;
        }
    }

    return info;
}
