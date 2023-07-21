#include "Buffer.hpp"
#include "Image.hpp"

class ModelManager {
    std::optional<ReadonlyBuffer> modelPosVertBuffer;
    std::optional<ReadonlyBuffer> modelNormVertBuffer;
    std::optional<ReadonlyBuffer> modelTexcoordVertBuffer;
    std::optional<ReadonlyBuffer> modelJointsVertBuffer;
    std::optional<ReadonlyBuffer> modelWeightsVertBuffer;
    std::optional<ReadonlyBuffer> modelIndexBuffer;
    std::vector<ReadonlyImage> textureAtlas;
};
