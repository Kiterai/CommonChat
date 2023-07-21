#ifndef SIMPLERENDERPROC_HPP
#define SIMPLERENDERPROC_HPP

#include "../Render.hpp"
#include "../Helper.hpp"
#include <future>

class SimpleRenderProc : public IRenderProc {
    vk::Device device;
    vk::UniquePipelineLayout pipelinelayout;
    std::vector<vk::UniqueShaderModule> shaders;

    vk::UniquePipeline createPipeline(vk::Device device, vk::Extent2D extent, vk::RenderPass renderpass, vk::PipelineLayout pipelineLayout);
  public:
    SimpleRenderProc(vk::Device _device, vk::DescriptorSetLayout descLayout);
    RenderProcRenderTargetDependant prepareRenderTargetDependant(const RenderTarget &rt) override;
    void render(const RenderDetails &rd, const RenderTarget &rt, const RenderProcRenderTargetDependant &rprtd) override;
    ~SimpleRenderProc();
};

#endif // SIMPLERENDERPROC_HPP

