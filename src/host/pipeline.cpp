#include "pipeline.hpp"
#include <fstream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace oray {

Pipeline::Pipeline(Device &device, const std::string &vertFilepath,
                   const std::string &fragFilepath,
                   const PipelineConfigInfo &configInfo)
    : device(device) {
  createPipeline(vertFilepath, fragFilepath, configInfo);
}

std::vector<char> Pipeline::readFile(const std::string &filepath) {
  std::ifstream file{filepath, std::ios::ate | std::ios::binary};
  if (!file.is_open()) {
    std::runtime_error("failed to open file: " + filepath);
  }
  size_t size = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(size);

  file.seekg(0);
  file.read(buffer.data(), size);
  file.close();
  return buffer;
}

void Pipeline::createPipeline(const std::string &vertFilepath,
                              const std::string &fragFilepath,
                              const PipelineConfigInfo &configInfo) {
  auto vertCode = readFile(vertFilepath);
  auto fragCode = readFile(fragFilepath);
}

void Pipeline::createShaderModule(const std::vector<char> &code,
                                  VkShaderModule *shaderModule) {
  VkShaderModuleCreateInfo createInfo{
      VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  if (vkCreateShaderModule(device.device(), &createInfo, nullptr,
                           shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("error creating shader module!");
  }
}

PipelineConfigInfo Pipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) {
    PipelineConfigInfo configInfo{};
    return configInfo;
}

} // namespace oray