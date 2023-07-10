#include <vulkan/vulkan.hpp>

struct RenderTargetHint {
    vk::Format format;
    vk::Extent2D extent;
    std::vector<vk::Image> images;
};

struct RenderProcDependant {
    vk::UniqueDescriptorSetLayout descLayout;
    std::vector<vk::UniqueDescriptorSet> descSets;
    std::vector<vk::UniqueShaderModule> shaders;
    vk::UniquePipelineLayout pipelinelayout;
    RenderProcDependant(RenderProcDependant &&) = default;
};
struct RenderProcRenderTargetDependant {
    std::vector<vk::UniqueFramebuffer> frameBufs;
    vk::UniqueRenderPass renderpass;
    vk::UniquePipeline pipeline;
    RenderProcRenderTargetDependant(RenderProcRenderTargetDependant &&) = default;
};

struct RenderTarget {
    vk::UniqueRenderPass renderpass; // no longer included
    vk::UniquePipeline pipeline;     // no longer included

    vk::Extent2D extent;
    std::vector<vk::UniqueImageView> imageViews;
    std::vector<vk::UniqueFramebuffer> frameBufs; // no longer included
};

class IRenderProc {
  public:
    virtual RenderProcDependant prepareDependant() = 0;
    virtual RenderProcRenderTargetDependant prepareRenderTargetDependant(const RenderTarget &target) = 0;
    virtual void render(const RenderProcDependant &rpd, const RenderProcRenderTargetDependant &rprtd) = 0;
};
