#pragma once

#include "device.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {

struct PipelineConfigInfo {
  PipelineConfigInfo(const PipelineConfigInfo &) = delete;
  PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

  VkPipelineViewportStateCreateInfo viewportInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  VkPipelineRasterizationStateCreateInfo rasterizationInfo;
  VkPipelineMultisampleStateCreateInfo multisampleInfo;
  VkPipelineColorBlendAttachmentState colorBlendAttachment;
  VkPipelineColorBlendStateCreateInfo colorBlendInfo;
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
  std::vector<VkDynamicState> dynamicStateEnables;
  VkPipelineDynamicStateCreateInfo dynamicStateInfo;
  VkPipelineLayout pipelineLayout = nullptr;
  VkRenderPass renderPass = nullptr;
  uint32_t subpass = 0;
};

class Pipeline {
public:
  Pipeline(
      Device &device, const std::string &vertFilepath,
      const std::string &fragFilepath, const PipelineConfigInfo &configInfo,
      const std::vector<VkVertexInputBindingDescription> &bindingDescriptions,
      const std::vector<VkVertexInputAttributeDescription>
          &attributeDescriptions);
  ~Pipeline();

  Pipeline(const Pipeline &) = delete;
  Pipeline &operator=(const Pipeline &) = delete;

  static void defaultPipelineConfigInfo(PipelineConfigInfo &configInfo);

  void bind(VkCommandBuffer commandBuffer);

  static std::vector<char> readFile(const std::string &filepath);

private:
  void createPipeline(const std::string &vertFilepath,
                      const std::string &fragFilepath,
                      const PipelineConfigInfo &configInfo);

  void createShaderModule(const std::vector<char> &code,
                          VkShaderModule *shaderModule);

  Device &device;

  VkPipeline pipeline;
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
  std::vector<VkVertexInputBindingDescription> bindingDescriptions;
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};
} // namespace oray