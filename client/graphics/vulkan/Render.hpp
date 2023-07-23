#ifndef RENDER_HPP
#define RENDER_HPP

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include "Image.hpp"

struct RenderTargetHint {
    vk::Format format;
    vk::Extent2D extent;
    std::vector<vk::Image> images;
};

struct RenderProcRenderTargetDependant {
    std::vector<Image> depthImages;
    std::vector<vk::UniqueImageView> depthImageViews;
    std::vector<vk::UniqueFramebuffer> frameBufs;
    vk::UniqueRenderPass renderpass;
    vk::UniquePipeline pipeline;
};

struct RenderTarget {
    vk::Extent2D extent;
    vk::Format format;
    std::vector<vk::UniqueImageView> imageViews;
};

struct RenderDetails {
    vk::CommandBuffer cmdBuf;
    uint32_t imageIndex, modelsCount;

    // vertex buffers
    vk::Buffer positionVertBuf, normalVertBuf, tangentVertBuf;
    vk::Buffer texcoordVertBuf[4], colorVertBuf[4], jointsVertBuf[4], weightsVertBuf[4];

    vk::Buffer indexBuf, drawBuf;

    uint32_t drawBufOffset;
    uint32_t drawBufStride;

    vk::DescriptorSet descSet;

    std::array<uint32_t, 1> dynamicOfs;
};

struct SceneData {
    glm::mat4x4 view;
    glm::mat4x4 proj;
};

class IRenderProc {
  public:
    virtual RenderProcRenderTargetDependant prepareRenderTargetDependant(const RenderTarget &target) = 0;
    virtual void render(const RenderDetails &rd, const RenderTarget &rt, const RenderProcRenderTargetDependant &rprtd) = 0;
    virtual ~IRenderProc() {};
};

#endif
