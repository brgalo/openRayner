#pragma once

#include "device.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oray {

struct PipelineConfigInfo {};

class Pipeline {
public:
  Pipeline(Device &device, const std::string &vertFilepath,
           const std::string &fragFilepath,
           const PipelineConfigInfo &configInfo);
  ~Pipeline(){};

  Pipeline(const Pipeline &) = delete;
  void operator=(const Pipeline &) = delete;

  static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width,
                                                      uint32_t height);

private:
  static std::vector<char> readFile(const std::string &filepath);
  void createPipeline(const std::string &vertFilePath,
                      const std::string &fragFilePath,
                      const PipelineConfigInfo &configInfo);

  void createShaderModule(const std::vector<char> &code,
                          VkShaderModule *shaderModule);

  Device &device;
  VkPipeline graphicsPipeline;
  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;
};
} // namespace oray