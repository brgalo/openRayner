#include "gui.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "implot.h"
#include "state.hpp"
#include <stdexcept>


namespace oray {

Gui::Gui(Device &device, GLFWwindow *pWindow, SwapChain *swapchain, std::shared_ptr<State> state)
    : device{device}, window{pWindow}, state{state} {

  createDescriptorPool();
  createContext(swapchain->getSwapChainImageFormat(), swapchain->imageCount());
  uploadFonts();
  createFramebuffers(swapchain);
}

Gui::~Gui() {
  vkDeviceWaitIdle(device.device());
  for (auto &frameBuffer : frameBuffers) {
    vkDestroyFramebuffer(device.device(), frameBuffer, nullptr);
  }
  destroyImGuiRenderPass();
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
}

void Gui::recordImGuiCommands(VkCommandBuffer buffer, uint32_t imgIdx,
                              VkExtent2D extent) {
  bool val_changed = false;
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::ShowDemoWindow();
  ImPlot::ShowMetricsWindow();

  ImGui::Begin("Test");
  ImGui::SliderFloat("Line Width", &state->lineWidth, 0.2f, 10.f);
  state->doTrace |= ImGui::SliderInt("nRays", &state->nRays, 0, 5000);

  ImGui::Combo("select triangle:", &state->currTri, &State::itemGetter,
               state->triNames.data(), state->triNames.size());

  state->doTrace |= ImGui::Button("go");


  state->HAS_CHANGED = val_changed;

  ImGui::End();


  ImGui::Render();

  VkRenderPassBeginInfo renderPassInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = frameBuffers[imgIdx];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = extent;
  vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer);

  vkCmdEndRenderPass(buffer);
}

void Gui::createContext(VkFormat imageFormat, uint32_t imageCount) {
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  // init implot
  ImPlot::CreateContext();
  ImPlot::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(window, true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = device.getInstance();
  init_info.PhysicalDevice = device.getPhysicalDevice();
  init_info.Device = device.device();
  init_info.QueueFamily = device.findPhysicalQueueFamilies().graphicsFamily;
  init_info.Queue = device.graphicsQueue();
  init_info.PipelineCache = VK_NULL_HANDLE;
  init_info.DescriptorPool = descriptorPool;
  init_info.Allocator = nullptr;
  init_info.MinImageCount = 2;
  init_info.ImageCount = imageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

  createImGuiRenderPass(imageFormat);

  ImGui_ImplVulkan_Init(&init_info, renderPass);
}

void Gui::createDescriptorPool() {
  // allocate a humungous descriptor pool?
  // TODO: reduce size

  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  if (vkCreateDescriptorPool(device.device(), &pool_info, nullptr,
                             &descriptorPool) != VK_SUCCESS) {
    throw std::runtime_error(
        "couldnt allocate decriptor pool for imgui/implot");
  }
}

void Gui::createFramebuffers(SwapChain *swapchain) {
  frameBuffers.resize(swapchain->imageCount());
  VkFramebufferCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.renderPass = renderPass;
  info.attachmentCount = 1;
  info.width = swapchain->width();
  info.height = swapchain->height();
  info.layers = 1;
  for (int i = 0; i < swapchain->imageCount(); i++) {
    info.pAttachments = swapchain->getImageViewPointer(i);
    if (vkCreateFramebuffer(device.device(), &info, nullptr,
                            &frameBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("couldnt create image view Framebuffers!");
    }
  }
}

void Gui::createImGuiRenderPass(VkFormat imageFormat) {
  VkAttachmentDescription attachment = {};
  attachment.format = imageFormat;
  attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachment = {};
  colorAttachment.attachment = 0;
  colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachment;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  info.attachmentCount = 1;
  info.pAttachments = &attachment;
  info.subpassCount = 1;
  info.pSubpasses = &subpass;
  info.dependencyCount = 1;
  info.pDependencies = &dependency;

  if (vkCreateRenderPass(device.device(), &info, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create gui render pass!");
  }
}

void Gui::destroyImGuiRenderPass() {
  vkDestroyRenderPass(device.device(), renderPass, nullptr);
}

void Gui::destroyFramebuffers() {
  for (auto &buffer : frameBuffers) {
    vkDestroyFramebuffer(device.device(), buffer, nullptr);
  }
}
void Gui::uploadFonts() {
  VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();
  ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
  device.endSingleTimeCommands(commandBuffer);
  vkDeviceWaitIdle(device.device());
  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Gui::recreateFramebuffers(SwapChain *swapchain) {
  destroyFramebuffers();
  createFramebuffers(swapchain);
}
} // namespace oray
