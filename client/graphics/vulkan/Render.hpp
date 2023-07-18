#ifndef RENDER_HPP
#define RENDER_HPP

#include <vulkan/vulkan.hpp>

struct RenderTargetHint {
    vk::Format format;
    vk::Extent2D extent;
    std::vector<vk::Image> images;
};

struct RenderProcRenderTargetDependant {
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
    vk::Buffer vertBuf, indexBuf, drawBuf;
    vk::DescriptorSet descSet;
};

class IRenderProc {
  public:
    virtual RenderProcRenderTargetDependant prepareRenderTargetDependant(const RenderTarget &target) = 0;
    virtual void render(const RenderDetails &rd, const RenderTarget &rt, const RenderProcRenderTargetDependant &rprtd) = 0;
    virtual ~IRenderProc() {};
};

#endif
