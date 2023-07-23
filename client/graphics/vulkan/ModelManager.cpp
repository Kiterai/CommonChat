#include "Modelmanager.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "Render.hpp"
#include <glm/glm.hpp>

constexpr uint32_t maxVertNum = 1048576;
constexpr uint32_t maxIndNum = 4194304;
constexpr uint32_t maxTexNum = 4194304;

ModelManager::ModelManager(vk::PhysicalDevice physDevice, vk::Device device) : physDevice{physDevice}, device{device} {
    modelPosVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::vec3) * maxVertNum);
    modelNormVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::vec3) * maxVertNum);
    modelTexcoordVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::vec2) * maxVertNum);
    modelJointsVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::u16vec4) * maxVertNum);
    modelWeightsVertBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eVertexBuffer, sizeof(glm::vec4) * maxVertNum);
    modelIndexBuffer.emplace(physDevice, device, vk::BufferUsageFlagBits::eIndexBuffer, sizeof(uint32_t) * maxIndNum);
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
}

ModelManager::ModelInfo ModelManager::loadModelFromGlbFile(const std::filesystem::path path, vk::Queue queue, vk::CommandBuffer cmdBuf, vk::Fence fence) {
    fastgltf::GltfDataBuffer buffer;
    buffer.loadFromFile(path);
    auto gltf = gltfParser.loadBinaryGLTF(&buffer, path.parent_path());
    gltf->parse();
    if (auto err = gltf->validate(); err != fastgltf::Error::None)
        throw std::runtime_error("error on load glb");
    auto asset = gltf->getParsedAsset();

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
    info.jointNum = asset->nodes.size();

    MeshPointer pPrimitiveBase = allocate(vertNumSum, indNumSum);
    MeshPointer pCurrentPrimitive = pPrimitiveBase;
    for (const auto &mesh : asset->meshes) {
        for (const auto &primitive : mesh.primitives) {
            for (const auto &[attrName, attrIndex] : primitive.attributes) {
                const auto &accessor = asset->accessors[attrIndex];
                const auto &bufferView = asset->bufferViews[accessor.bufferViewIndex.value()];
                const auto &bufferBytes = std::get<fastgltf::sources::ByteView>(asset->buffers[bufferView.bufferIndex].data).bytes;
                if (attrName == "POSITION") {
                    modelPosVertBuffer->write(physDevice, device, queue, cmdBuf,
                                              static_cast<const void *>(bufferBytes.data() + bufferView.byteOffset), bufferView.byteLength,
                                              pCurrentPrimitive.vertexBase * sizeof(glm::vec3), fence);
                } else if (attrName == "NORMAL") {
                    modelNormVertBuffer->write(physDevice, device, queue, cmdBuf,
                                               static_cast<const void *>(bufferBytes.data() + bufferView.byteOffset), bufferView.byteLength,
                                               pCurrentPrimitive.vertexBase * sizeof(glm::vec3), fence);
                } else if (attrName == "TEXCOORD_0") {
                    modelTexcoordVertBuffer->write(physDevice, device, queue, cmdBuf,
                                                   static_cast<const void *>(bufferBytes.data() + bufferView.byteOffset), bufferView.byteLength,
                                                   pCurrentPrimitive.vertexBase * sizeof(glm::vec2), fence);
                } else if (attrName == "JOINTS_0") {
                    modelJointsVertBuffer->write(physDevice, device, queue, cmdBuf,
                                                 static_cast<const void *>(bufferBytes.data() + bufferView.byteOffset), bufferView.byteLength,
                                                 pCurrentPrimitive.vertexBase * sizeof(glm::u16vec4), fence);
                } else if (attrName == "WEIGHTS_0") {
                    modelWeightsVertBuffer->write(physDevice, device, queue, cmdBuf,
                                                  static_cast<const void *>(bufferBytes.data() + bufferView.byteOffset), bufferView.byteLength,
                                                  pCurrentPrimitive.vertexBase * sizeof(glm::vec4), fence);
                }
            }
            {
                const auto &accessor = asset->accessors[primitive.indicesAccessor.value()];
                const auto &bufferView = asset->bufferViews[accessor.bufferViewIndex.value()];
                const auto &bufferBytes = std::get<fastgltf::sources::ByteView>(asset->buffers[bufferView.bufferIndex].data).bytes;
                modelIndexBuffer->write(physDevice, device, queue, cmdBuf,
                                        static_cast<const void *>(bufferBytes.data() + bufferView.byteOffset), bufferView.byteLength,
                                        pCurrentPrimitive.IndexBase * sizeof(uint32_t), fence);
            }

            pCurrentPrimitive.indexNum = asset->accessors[primitive.indicesAccessor.value()].count;
            pCurrentPrimitive.materialIndex = pPrimitiveBase.materialIndex + primitive.materialIndex.value();
            pCurrentPrimitive.textureIndex = pPrimitiveBase.textureIndex + asset->materials[primitive.materialIndex.value()].pbrData->baseColorTexture->textureIndex;
            info.primitives.push_back(pCurrentPrimitive);

            pCurrentPrimitive.vertexBase += asset->accessors[primitive.attributes.begin()->second].count;
            pCurrentPrimitive.IndexBase += asset->accessors[primitive.indicesAccessor.value()].count;
        }
    }

    return info;
}
