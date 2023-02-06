#include "compute.hpp"
#include "pipeline.hpp"
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {
void Compute::createPipelineLayout() {
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(Compute::PushConsts);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo{
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline layout!");
  }
}

void Compute::createPipeline() {
  auto computeCode = Pipeline::readFile("sumPerMesh.comp.spv");
  VkShaderModule computeShaderModule;
  Pipeline::createShaderModule(device, computeCode, &computeShaderModule);
  
  VkPipelineShaderStageCreateInfo shaderStage[1];
  shaderStage[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage[0].stage = VK_SHADER_STAGE_COMPUTE_BIT;
  shaderStage[0].module = computeShaderModule;
  shaderStage[0].pName = "main";
  shaderStage[0].flags = 0;
  shaderStage[0].pNext = nullptr;
  shaderStage[0].pSpecializationInfo = nullptr;

  VkComputePipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipelineInfo.pNext = nullptr;
  pipelineInfo.flags = 0;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.basePipelineIndex = -1;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateComputePipelines(device.device(), nullptr, 1, &pipelineInfo,
                               nullptr, &pipeline) != VK_SUCCESS) {
    throw std::runtime_error("failed to create compute pipeline!");
  }
}

Compute::Compute(Device &device, std::vector<OrayObject> const &orayObjects) : device{device} {
  pushConsts.nMeshes = orayObjects[0].geom->triToMeshIdx().size();
  pushConsts.nTriangleToMeshIdxBuffer = orayObjects[0].geom->getTriangleToMeshIdxBufferAddress();
}
}