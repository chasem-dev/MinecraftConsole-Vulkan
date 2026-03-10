#include "VulkanBootstrapApp.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>
#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr size_t kInitialFrameVertices = 262144;
constexpr VkFormat kDepthFormat = VK_FORMAT_D32_SFLOAT;

std::vector<char> readBinaryFile(const std::string &path)
{
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open())
  {
    throw std::runtime_error("Failed to open shader file: " + path);
  }

  const std::streamsize fileSize = file.tellg();
  std::vector<char> buffer(static_cast<size_t>(fileSize));
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  return buffer;
}

VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code)
{
  VkShaderModuleCreateInfo createInfo {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateShaderModule failed");
  }

  return shaderModule;
}

std::string buildShaderPath(const char *fileName)
{
  return std::string(MCE_SHADER_DIR) + "/" + fileName;
}

VkSampler createSampler(
  VkDevice device,
  VkFilter filter,
  VkSamplerAddressMode addressMode)
{
  VkSamplerCreateInfo samplerInfo {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = filter;
  samplerInfo.minFilter = filter;
  samplerInfo.mipmapMode = filter == VK_FILTER_LINEAR
    ? VK_SAMPLER_MIPMAP_MODE_LINEAR
    : VK_SAMPLER_MIPMAP_MODE_NEAREST;
  samplerInfo.addressModeU = addressMode;
  samplerInfo.addressModeV = addressMode;
  samplerInfo.addressModeW = addressMode;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;
  samplerInfo.mipLodBias = 0.0f;

  VkSampler sampler = VK_NULL_HANDLE;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateSampler failed");
  }

  return sampler;
}
}

void VulkanBootstrapApp::recreateSwapchain()
{
  int framebufferWidth = 0;
  int framebufferHeight = 0;
  while (framebufferWidth == 0 || framebufferHeight == 0)
  {
    glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device_);

  cleanupSwapchain();
  createSwapchain();
  createImageViews();
  createDepthResources();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandBuffers();
}

void VulkanBootstrapApp::cleanupSwapchain()
{
  if (!commandBuffers_.empty())
  {
    vkFreeCommandBuffers(
      device_,
      commandPool_,
      static_cast<uint32_t>(commandBuffers_.size()),
      commandBuffers_.data());
    commandBuffers_.clear();
  }

  for (VkFramebuffer framebuffer : swapchainFramebuffers_)
  {
    vkDestroyFramebuffer(device_, framebuffer, nullptr);
  }
  swapchainFramebuffers_.clear();

  for (VkPipeline &pipeline : pipelines_)
  {
    if (pipeline != VK_NULL_HANDLE)
    {
      vkDestroyPipeline(device_, pipeline, nullptr);
      pipeline = VK_NULL_HANDLE;
    }
  }

  if (pipelineLayout_ != VK_NULL_HANDLE)
  {
    vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
    pipelineLayout_ = VK_NULL_HANDLE;
  }
  if (renderPass_ != VK_NULL_HANDLE)
  {
    vkDestroyRenderPass(device_, renderPass_, nullptr);
    renderPass_ = VK_NULL_HANDLE;
  }

  if (depthImageView_ != VK_NULL_HANDLE)
  {
    vkDestroyImageView(device_, depthImageView_, nullptr);
    depthImageView_ = VK_NULL_HANDLE;
  }
  if (depthImage_ != VK_NULL_HANDLE)
  {
    vkDestroyImage(device_, depthImage_, nullptr);
    depthImage_ = VK_NULL_HANDLE;
  }
  if (depthImageMemory_ != VK_NULL_HANDLE)
  {
    vkFreeMemory(device_, depthImageMemory_, nullptr);
    depthImageMemory_ = VK_NULL_HANDLE;
  }

  for (VkImageView imageView : swapchainImageViews_)
  {
    vkDestroyImageView(device_, imageView, nullptr);
  }
  swapchainImageViews_.clear();

  if (swapchain_ != VK_NULL_HANDLE)
  {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
  }
  swapchainImages_.clear();
}

void VulkanBootstrapApp::createSwapchain()
{
  const SwapchainSupportDetails support = querySwapchainSupport(physicalDevice_);
  const VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(support.formats);
  const VkPresentModeKHR presentMode = chooseSwapPresentMode(support.presentModes);
  const VkExtent2D extent = chooseSwapExtent(support.capabilities);

  uint32_t imageCount = support.capabilities.minImageCount + 1;
  if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
  {
    imageCount = support.capabilities.maxImageCount;
  }

  const QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);
  const uint32_t queueFamilyIndices[] = {
    indices.graphicsFamily.value(),
    indices.presentFamily.value()
  };

  VkSwapchainCreateInfoKHR createInfo {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface_;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  if (indices.graphicsFamily != indices.presentFamily)
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = support.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;

  if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapchain_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateSwapchainKHR failed");
  }

  vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
  swapchainImages_.resize(imageCount);
  vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data());

  swapchainImageFormat_ = surfaceFormat.format;
  swapchainExtent_ = extent;
  activePresentMode_ = presentMode;
  logSwapchainSelection(surfaceFormat, presentMode, extent);
}

void VulkanBootstrapApp::createImageViews()
{
  swapchainImageViews_.resize(swapchainImages_.size());

  for (size_t index = 0; index < swapchainImages_.size(); ++index)
  {
    VkImageViewCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapchainImages_[index];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapchainImageFormat_;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &createInfo, nullptr, &swapchainImageViews_[index]) != VK_SUCCESS)
    {
      throw std::runtime_error("vkCreateImageView failed");
    }
  }
}

void VulkanBootstrapApp::createDepthResources()
{
  VkImageCreateInfo imageInfo {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = swapchainExtent_.width;
  imageInfo.extent.height = swapchainExtent_.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = kDepthFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device_, &imageInfo, nullptr, &depthImage_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateImage failed for depth buffer");
  }

  VkMemoryRequirements memoryRequirements {};
  vkGetImageMemoryRequirements(device_, depthImage_, &memoryRequirements);

  VkMemoryAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memoryRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
    memoryRequirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &depthImageMemory_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkAllocateMemory failed for depth buffer");
  }

  vkBindImageMemory(device_, depthImage_, depthImageMemory_, 0);

  VkImageViewCreateInfo viewInfo {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = depthImage_;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = kDepthFormat;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device_, &viewInfo, nullptr, &depthImageView_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateImageView failed for depth buffer");
  }
}

void VulkanBootstrapApp::createRenderPass()
{
  VkAttachmentDescription colorAttachment {};
  colorAttachment.format = swapchainImageFormat_;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentDescription depthAttachment {};
  depthAttachment.format = kDepthFormat;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentRef {};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef {};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask =
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstStageMask =
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask =
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  const std::array<VkAttachmentDescription, 2> attachments {{
    colorAttachment,
    depthAttachment
  }};

  VkRenderPassCreateInfo renderPassInfo {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateRenderPass failed");
  }
}

void VulkanBootstrapApp::createGraphicsPipeline()
{
  const std::vector<char> colorVertexShaderCode = readBinaryFile(buildShaderPath("mce_color.vert.spv"));
  const std::vector<char> colorFragmentShaderCode = readBinaryFile(buildShaderPath("mce_color.frag.spv"));
  const std::vector<char> texturedVertexShaderCode = readBinaryFile(buildShaderPath("mce_textured.vert.spv"));
  const std::vector<char> texturedFragmentShaderCode = readBinaryFile(buildShaderPath("mce_textured.frag.spv"));
  const std::vector<char> alphaTestFragmentShaderCode = readBinaryFile(buildShaderPath("mce_textured_alphatest.frag.spv"));
  const std::vector<char> fogFragmentShaderCode = readBinaryFile(buildShaderPath("mce_textured_fog.frag.spv"));
  const std::vector<char> fogAlphaTestFragmentShaderCode = readBinaryFile(buildShaderPath("mce_textured_fog_alphatest.frag.spv"));

  const VkShaderModule colorVertexShaderModule = createShaderModule(device_, colorVertexShaderCode);
  const VkShaderModule colorFragmentShaderModule = createShaderModule(device_, colorFragmentShaderCode);
  const VkShaderModule texturedVertexShaderModule = createShaderModule(device_, texturedVertexShaderCode);
  const VkShaderModule texturedFragmentShaderModule = createShaderModule(device_, texturedFragmentShaderCode);
  const VkShaderModule alphaTestFragmentShaderModule = createShaderModule(device_, alphaTestFragmentShaderCode);
  const VkShaderModule fogFragmentShaderModule = createShaderModule(device_, fogFragmentShaderCode);
  const VkShaderModule fogAlphaTestFragmentShaderModule = createShaderModule(device_, fogAlphaTestFragmentShaderCode);

  VkVertexInputBindingDescription bindingDescription {};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  const std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions {{
    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
    {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)},
    {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color)}
  }};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = nullptr;
  viewportState.scissorCount = 1;
  viewportState.pScissors = nullptr;

  const std::array<VkDynamicState, 2> dynamicStates {{
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  }};
  VkPipelineDynamicStateCreateInfo dynamicState {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkPipelineRasterizationStateCreateInfo rasterizer {};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling {};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPushConstantRange pushConstantRange {};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(float) * 16;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = textureSetLayout_ != VK_NULL_HANDLE ? 1u : 0u;
  pipelineLayoutInfo.pSetLayouts = textureSetLayout_ != VK_NULL_HANDLE ? &textureSetLayout_ : nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreatePipelineLayout failed");
  }

  auto createPipelineForConfig =
    [&](ShaderVariant variant,
        BlendMode blendMode,
        bool depthTestEnabled,
        bool depthWriteEnabled,
        bool cullEnabled,
        bool cullClockwise) {
      VkShaderModule vertexShaderModule = variant == ShaderVariant::ColorOnly
        ? colorVertexShaderModule
        : texturedVertexShaderModule;
      VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
      switch (variant)
      {
      case ShaderVariant::ColorOnly:
        fragmentShaderModule = colorFragmentShaderModule;
        break;
      case ShaderVariant::Textured:
        fragmentShaderModule = texturedFragmentShaderModule;
        break;
      case ShaderVariant::TexturedAlphaTest:
        fragmentShaderModule = alphaTestFragmentShaderModule;
        break;
      case ShaderVariant::TexturedFog:
        fragmentShaderModule = fogFragmentShaderModule;
        break;
      case ShaderVariant::TexturedFogAlphaTest:
        fragmentShaderModule = fogAlphaTestFragmentShaderModule;
        break;
      default:
        throw std::runtime_error("Unsupported shader variant");
      }

      VkPipelineShaderStageCreateInfo vertexShaderStageInfo {};
      vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
      vertexShaderStageInfo.module = vertexShaderModule;
      vertexShaderStageInfo.pName = "main";

      VkPipelineShaderStageCreateInfo fragmentShaderStageInfo {};
      fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
      fragmentShaderStageInfo.module = fragmentShaderModule;
      fragmentShaderStageInfo.pName = "main";

      const VkPipelineShaderStageCreateInfo shaderStages[] = {
        vertexShaderStageInfo,
        fragmentShaderStageInfo
      };

      VkPipelineColorBlendAttachmentState colorBlendAttachment {};
      colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
      colorBlendAttachment.blendEnable = blendMode != BlendMode::Opaque ? VK_TRUE : VK_FALSE;
      if (blendMode == BlendMode::Alpha)
      {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
      }
      else if (blendMode == BlendMode::Additive)
      {
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
      }

      VkPipelineColorBlendStateCreateInfo colorBlending {};
      colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
      colorBlending.logicOpEnable = VK_FALSE;
      colorBlending.attachmentCount = 1;
      colorBlending.pAttachments = &colorBlendAttachment;

      VkPipelineDepthStencilStateCreateInfo depthStencil {};
      depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
      depthStencil.depthTestEnable = depthTestEnabled ? VK_TRUE : VK_FALSE;
      depthStencil.depthWriteEnable = depthWriteEnabled ? VK_TRUE : VK_FALSE;
      depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
      depthStencil.depthBoundsTestEnable = VK_FALSE;
      depthStencil.stencilTestEnable = VK_FALSE;

      VkPipelineRasterizationStateCreateInfo rasterizerState = rasterizer;
      rasterizerState.cullMode = cullEnabled ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
      // We use a negative-height viewport when recording command buffers to
      // match OpenGL's screen-space orientation. That flips winding, so front
      // face must be inverted here to preserve legacy GL cull semantics.
      rasterizerState.frontFace = cullClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;

      VkGraphicsPipelineCreateInfo pipelineInfo {};
      pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      pipelineInfo.stageCount = 2;
      pipelineInfo.pStages = shaderStages;
      pipelineInfo.pVertexInputState = &vertexInputInfo;
      pipelineInfo.pInputAssemblyState = &inputAssembly;
      pipelineInfo.pViewportState = &viewportState;
      pipelineInfo.pRasterizationState = &rasterizerState;
      pipelineInfo.pMultisampleState = &multisampling;
      pipelineInfo.pDepthStencilState = &depthStencil;
      pipelineInfo.pColorBlendState = &colorBlending;
      pipelineInfo.pDynamicState = &dynamicState;
      pipelineInfo.layout = pipelineLayout_;
      pipelineInfo.renderPass = renderPass_;
      pipelineInfo.subpass = 0;

      VkPipeline pipeline = VK_NULL_HANDLE;
      if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
      {
        throw std::runtime_error("vkCreateGraphicsPipelines failed");
      }
      return pipeline;
    };

  for (uint32_t variantIndex = 0; variantIndex < static_cast<uint32_t>(ShaderVariant::Count); ++variantIndex)
  {
    const ShaderVariant variant = static_cast<ShaderVariant>(variantIndex);
    for (uint32_t blendIndex = 0; blendIndex < static_cast<uint32_t>(BlendMode::Count); ++blendIndex)
    {
      const BlendMode blendMode = static_cast<BlendMode>(blendIndex);
      for (uint32_t depthTestIndex = 0; depthTestIndex < 2; ++depthTestIndex)
      {
        for (uint32_t depthWriteIndex = 0; depthWriteIndex < 2; ++depthWriteIndex)
        {
          for (uint32_t cullIndex = 0; cullIndex < 2; ++cullIndex)
          {
            for (uint32_t clockwiseIndex = 0; clockwiseIndex < 2; ++clockwiseIndex)
            {
              const uint32_t pipelineIndex = getPipelineIndex(
                variant,
                blendMode,
                depthTestIndex != 0,
                depthWriteIndex != 0,
                cullIndex != 0,
                clockwiseIndex != 0);
              pipelines_[pipelineIndex] = createPipelineForConfig(
                variant,
                blendMode,
                depthTestIndex != 0,
                depthWriteIndex != 0,
                cullIndex != 0,
                clockwiseIndex != 0);
            }
          }
        }
      }
    }
  }

  vkDestroyShaderModule(device_, fogAlphaTestFragmentShaderModule, nullptr);
  vkDestroyShaderModule(device_, fogFragmentShaderModule, nullptr);
  vkDestroyShaderModule(device_, alphaTestFragmentShaderModule, nullptr);
  vkDestroyShaderModule(device_, texturedFragmentShaderModule, nullptr);
  vkDestroyShaderModule(device_, texturedVertexShaderModule, nullptr);
  vkDestroyShaderModule(device_, colorFragmentShaderModule, nullptr);
  vkDestroyShaderModule(device_, colorVertexShaderModule, nullptr);
}

void VulkanBootstrapApp::createFramebuffers()
{
  swapchainFramebuffers_.resize(swapchainImageViews_.size());

  for (size_t index = 0; index < swapchainImageViews_.size(); ++index)
  {
    const std::array<VkImageView, 2> attachments {{
      swapchainImageViews_[index],
      depthImageView_
    }};

    VkFramebufferCreateInfo framebufferInfo {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass_;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapchainExtent_.width;
    framebufferInfo.height = swapchainExtent_.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapchainFramebuffers_[index]) != VK_SUCCESS)
    {
      throw std::runtime_error("vkCreateFramebuffer failed");
    }
  }
}

void VulkanBootstrapApp::createVertexBuffer(size_t vertexCapacity)
{
  const VkDeviceSize bufferSize = sizeof(Vertex) * vertexCapacity;
  createBuffer(
    bufferSize,
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    vertexBuffer_,
    vertexBufferMemory_);
  if (vkMapMemory(device_, vertexBufferMemory_, 0, bufferSize, 0, &vertexBufferMapped_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkMapMemory failed for vertex buffer");
  }
  vertexBufferCapacity_ = vertexCapacity;
}

void VulkanBootstrapApp::ensureVertexBufferCapacity(size_t requiredVertices)
{
  if (requiredVertices == 0 || requiredVertices <= vertexBufferCapacity_)
  {
    return;
  }

  size_t newCapacity = std::max(vertexBufferCapacity_, kInitialFrameVertices);
  while (newCapacity < requiredVertices)
  {
    newCapacity *= 2;
  }

  if (vertexBufferMapped_ != nullptr)
  {
    vkUnmapMemory(device_, vertexBufferMemory_);
    vertexBufferMapped_ = nullptr;
  }
  if (vertexBuffer_ != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device_, vertexBuffer_, nullptr);
    vertexBuffer_ = VK_NULL_HANDLE;
  }
  if (vertexBufferMemory_ != VK_NULL_HANDLE)
  {
    vkFreeMemory(device_, vertexBufferMemory_, nullptr);
    vertexBufferMemory_ = VK_NULL_HANDLE;
  }

  createVertexBuffer(newCapacity);
}

void VulkanBootstrapApp::createCommandBuffers()
{
  commandBuffers_.resize(swapchainFramebuffers_.size());

  VkCommandBufferAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool_;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

  if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS)
  {
    throw std::runtime_error("vkAllocateCommandBuffers failed");
  }
}

void VulkanBootstrapApp::createSyncObjects()
{
  VkSemaphoreCreateInfo semaphoreInfo {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  const bool semaphoreFailed =
    vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphore_) != VK_SUCCESS ||
    vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphore_) != VK_SUCCESS;
  if (semaphoreFailed || vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFence_) != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create synchronization primitives");
  }
}

void VulkanBootstrapApp::createTextureResources()
{
  createDescriptorSetLayout();
  createDescriptorPool();
  createSamplers();
  createFallbackTexture();
}

void VulkanBootstrapApp::createDescriptorPool()
{
  VkDescriptorPoolSize poolSize {};
  poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSize.descriptorCount = kMaxTextures;

  VkDescriptorPoolCreateInfo poolInfo {};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = kMaxTextures;

  if (vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateDescriptorPool failed");
  }
}

void VulkanBootstrapApp::createDescriptorSetLayout()
{
  VkDescriptorSetLayoutBinding samplerLayoutBinding {};
  samplerLayoutBinding.binding = 0;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &samplerLayoutBinding;

  if (vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &textureSetLayout_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateDescriptorSetLayout failed");
  }
}

void VulkanBootstrapApp::createSamplers()
{
  nearestRepeatSampler_ = createSampler(device_, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_REPEAT);
  nearestClampSampler_ = createSampler(device_, VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
  linearRepeatSampler_ = createSampler(device_, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
  linearClampSampler_ = createSampler(device_, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

void VulkanBootstrapApp::createFallbackTexture()
{
  fallbackTextureIndex_ = allocateTextureSlot();
  if (fallbackTextureIndex_ < 0)
  {
    throw std::runtime_error("Failed to allocate fallback texture slot");
  }

  const uint32_t whitePixel = 0xffffffffu;
  uploadTextureData(fallbackTextureIndex_, 1, 1, &whitePixel);
  boundTextureIndex_ = -1;
}

void VulkanBootstrapApp::drawFrame()
{
  auto frameStart = std::chrono::high_resolution_clock::now();

  std::vector<Vertex> frameVertices;
  std::vector<DrawBatch> frameBatches;
  {
    std::lock_guard<std::mutex> lock(frameDataMutex_);
    frameVertices = std::move(frameVertices_);
    frameBatches = std::move(frameBatches_);
  }

  auto fenceStart = std::chrono::high_resolution_clock::now();
  vkWaitForFences(device_, 1, &inFlightFence_, VK_TRUE, std::numeric_limits<uint64_t>::max());
  auto fenceEnd = std::chrono::high_resolution_clock::now();

  uint32_t imageIndex = 0;
  VkResult result = vkAcquireNextImageKHR(
    device_,
    swapchain_,
    std::numeric_limits<uint64_t>::max(),
    imageAvailableSemaphore_,
    VK_NULL_HANDLE,
    &imageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR)
  {
    recreateSwapchain();
    return;
  }
  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
  {
    throw std::runtime_error("vkAcquireNextImageKHR failed");
  }

  if (!frameVertices.empty())
  {
    ensureVertexBufferCapacity(frameVertices.size());
    std::memcpy(
      vertexBufferMapped_,
      frameVertices.data(),
      frameVertices.size() * sizeof(Vertex));
  }

  vkResetFences(device_, 1, &inFlightFence_);
  vkResetCommandBuffer(commandBuffers_[imageIndex], 0);
  recordCommandBuffer(commandBuffers_[imageIndex], imageIndex, frameBatches);

  const VkSemaphore waitSemaphores[] = {imageAvailableSemaphore_};
  const VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  const VkSemaphore signalSemaphores[] = {renderFinishedSemaphore_};

  VkSubmitInfo submitInfo {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFence_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkQueueSubmit failed");
  }

  VkPresentInfoKHR presentInfo {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain_;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(presentQueue_, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_)
  {
    framebufferResized_ = false;
    recreateSwapchain();
    return;
  }
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("vkQueuePresentKHR failed");
  }

  auto frameEnd = std::chrono::high_resolution_clock::now();
  frameStats_.drawFrameMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();
  frameStats_.fenceWaitMs = std::chrono::duration<double, std::milli>(fenceEnd - fenceStart).count();
  frameStats_.vertexCount = static_cast<uint32_t>(frameVertices.size());
  frameStats_.batchCount = static_cast<uint32_t>(frameBatches.size());

  uint32_t texCount = 0;
  for (uint32_t i = 0; i < kMaxTextures; ++i)
  {
    if (textureSlots_[i].allocated)
      ++texCount;
  }
  frameStats_.textureCount = texCount;
  frameStats_.swapchainImageCount = static_cast<uint32_t>(swapchainImages_.size());
  frameStats_.presentModeName =
    activePresentMode_ == VK_PRESENT_MODE_MAILBOX_KHR ? "MAILBOX" :
    activePresentMode_ == VK_PRESENT_MODE_FIFO_KHR ? "FIFO (vsync)" :
    activePresentMode_ == VK_PRESENT_MODE_IMMEDIATE_KHR ? "IMMEDIATE" :
    activePresentMode_ == VK_PRESENT_MODE_FIFO_RELAXED_KHR ? "FIFO_RELAXED" : "unknown";

  prevFrameVertexCount_ = frameVertices.size();
  prevFrameBatchCount_ = frameBatches.size();
}

void VulkanBootstrapApp::recordCommandBuffer(
  VkCommandBuffer commandBuffer,
  uint32_t imageIndex,
  const std::vector<DrawBatch> &frameBatches)
{
  VkCommandBufferBeginInfo beginInfo {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("vkBeginCommandBuffer failed");
  }

  const std::array<VkClearValue, 2> clearValues {{
    {{{clearColour_[0], clearColour_[1], clearColour_[2], clearColour_[3]}}},
    {.depthStencil = {1.0f, 0}}
  }};

  VkRenderPassBeginInfo renderPassInfo {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass_;
  renderPassInfo.framebuffer = swapchainFramebuffers_[imageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapchainExtent_;
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  if (!frameBatches.empty())
  {
    const VkBuffer vertexBuffers[] = {vertexBuffer_};
    const VkDeviceSize offsets[] = {0};

    for (const DrawBatch &batch : frameBatches)
    {
      const uint32_t viewportWidth = batch.viewportWidth != 0 ? batch.viewportWidth : swapchainExtent_.width;
      const uint32_t viewportHeight = batch.viewportHeight != 0 ? batch.viewportHeight : swapchainExtent_.height;
      const int viewportX = batch.viewportX;
      const int viewportY = batch.viewportY;

      VkViewport viewport {};
      viewport.x = static_cast<float>(viewportX);
      viewport.y = static_cast<float>(viewportY + static_cast<int>(viewportHeight));
      viewport.width = static_cast<float>(viewportWidth);
      viewport.height = -static_cast<float>(viewportHeight);
      viewport.minDepth = 0.0f;
      viewport.maxDepth = 1.0f;
      vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

      VkRect2D scissor {};
      scissor.offset = {viewportX, viewportY};
      scissor.extent = {viewportWidth, viewportHeight};
      vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

      if (batch.clearFlags != 0)
      {
        std::array<VkClearAttachment, 2> clearAttachments {};
        uint32_t clearAttachmentCount = 0;

        if ((batch.clearFlags & GL_COLOR_BUFFER_BIT) != 0)
        {
          VkClearAttachment &colorAttachment = clearAttachments[clearAttachmentCount++];
          colorAttachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
          colorAttachment.colorAttachment = 0;
          colorAttachment.clearValue.color = {
            {clearColour_[0], clearColour_[1], clearColour_[2], clearColour_[3]}
          };
        }

        if ((batch.clearFlags & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)) != 0)
        {
          VkClearAttachment &depthAttachment = clearAttachments[clearAttachmentCount++];
          depthAttachment.aspectMask = 0;
          if ((batch.clearFlags & GL_DEPTH_BUFFER_BIT) != 0)
          {
            depthAttachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
          }
          if ((batch.clearFlags & GL_STENCIL_BUFFER_BIT) != 0)
          {
            depthAttachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
          }
          depthAttachment.clearValue.depthStencil = {1.0f, 0};
        }

        if (clearAttachmentCount > 0)
        {
          VkClearRect clearRect {};
          clearRect.rect.offset = {viewportX, viewportY};
          clearRect.rect.extent = {viewportWidth, viewportHeight};
          clearRect.baseArrayLayer = 0;
          clearRect.layerCount = 1;
          vkCmdClearAttachments(
            commandBuffer,
            clearAttachmentCount,
            clearAttachments.data(),
            1,
            &clearRect);
        }
        continue;
      }

      vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

      const VkPipeline pipeline = getPipelineForBatch(batch);
      if (pipeline == VK_NULL_HANDLE)
      {
        continue;
      }

      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

      int textureIndex = batch.renderState.textureIndex;
      if (textureIndex < 0 || textureIndex >= static_cast<int>(kMaxTextures) || !textureSlots_[textureIndex].allocated)
      {
        textureIndex = fallbackTextureIndex_;
      }

      if (textureIndex >= 0 && textureSlots_[textureIndex].descriptorSet != VK_NULL_HANDLE)
      {
        vkCmdBindDescriptorSets(
          commandBuffer,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          pipelineLayout_,
          0,
          1,
          &textureSlots_[textureIndex].descriptorSet,
          0,
          nullptr);
      }

      vkCmdPushConstants(
        commandBuffer,
        pipelineLayout_,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(float) * batch.mvp.size(),
        batch.mvp.data());
      vkCmdDraw(commandBuffer, batch.vertexCount, 1, batch.firstVertex, 0);
    }
  }
  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("vkEndCommandBuffer failed");
  }
}

uint32_t VulkanBootstrapApp::findMemoryType(
  uint32_t typeFilter,
  VkMemoryPropertyFlags properties) const
{
  VkPhysicalDeviceMemoryProperties memoryProperties {};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProperties);

  for (uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index)
  {
    const bool typeMatches = (typeFilter & (1u << index)) != 0;
    const bool propertyMatches =
      (memoryProperties.memoryTypes[index].propertyFlags & properties) == properties;
    if (typeMatches && propertyMatches)
    {
      return index;
    }
  }

  throw std::runtime_error("Failed to find compatible Vulkan memory type");
}

void VulkanBootstrapApp::createBuffer(
  VkDeviceSize size,
  VkBufferUsageFlags usage,
  VkMemoryPropertyFlags properties,
  VkBuffer &buffer,
  VkDeviceMemory &bufferMemory)
{
  VkBufferCreateInfo bufferInfo {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateBuffer failed");
  }

  VkMemoryRequirements memoryRequirements {};
  vkGetBufferMemoryRequirements(device_, buffer, &memoryRequirements);

  VkMemoryAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memoryRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("vkAllocateMemory failed");
  }

  vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}

void VulkanBootstrapApp::copyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size)
{
  VkCommandBuffer commandBuffer = beginOneTimeCommands();

  VkBufferCopy copyRegion {};
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &copyRegion);

  endOneTimeCommands(commandBuffer);
}

VkCommandBuffer VulkanBootstrapApp::beginOneTimeCommands()
{
  VkCommandBufferAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool_;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  if (vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("vkAllocateCommandBuffers failed");
  }

  VkCommandBufferBeginInfo beginInfo {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    throw std::runtime_error("vkBeginCommandBuffer failed");
  }

  return commandBuffer;
}

void VulkanBootstrapApp::endOneTimeCommands(VkCommandBuffer commandBuffer)
{
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    throw std::runtime_error("vkEndCommandBuffer failed");
  }

  VkSubmitInfo submitInfo {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    throw std::runtime_error("vkQueueSubmit failed");
  }
  vkQueueWaitIdle(graphicsQueue_);
  vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void VulkanBootstrapApp::transitionImageLayout(
  VkCommandBuffer commandBuffer,
  VkImage image,
  VkImageLayout oldLayout,
  VkImageLayout newLayout)
{
  VkImageMemoryBarrier barrier {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
           newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else
  {
    throw std::runtime_error("Unsupported image layout transition");
  }

  vkCmdPipelineBarrier(
    commandBuffer,
    sourceStage,
    destinationStage,
    0,
    0,
    nullptr,
    0,
    nullptr,
    1,
    &barrier);
}

void VulkanBootstrapApp::ensureStagingBuffer(VkDeviceSize requiredSize)
{
  if (stagingBuffer_ != VK_NULL_HANDLE && stagingBufferSize_ >= requiredSize)
  {
    return;
  }

  if (stagingBuffer_ != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device_, stagingBuffer_, nullptr);
    stagingBuffer_ = VK_NULL_HANDLE;
  }
  if (stagingBufferMemory_ != VK_NULL_HANDLE)
  {
    vkFreeMemory(device_, stagingBufferMemory_, nullptr);
    stagingBufferMemory_ = VK_NULL_HANDLE;
  }

  stagingBufferSize_ = std::max<VkDeviceSize>(requiredSize, 4 * 1024 * 1024);
  createBuffer(
    stagingBufferSize_,
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    stagingBuffer_,
    stagingBufferMemory_);
}

void VulkanBootstrapApp::destroyTextureResources()
{
  for (uint32_t index = 0; index < kMaxTextures; ++index)
  {
    destroyTextureSlotResources(static_cast<int>(index), false);
    textureSlots_[index] = TextureSlot {};
  }
  boundTextureIndex_ = -1;
  fallbackTextureIndex_ = -1;

  if (stagingBuffer_ != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device_, stagingBuffer_, nullptr);
    stagingBuffer_ = VK_NULL_HANDLE;
  }
  if (stagingBufferMemory_ != VK_NULL_HANDLE)
  {
    vkFreeMemory(device_, stagingBufferMemory_, nullptr);
    stagingBufferMemory_ = VK_NULL_HANDLE;
  }
  stagingBufferSize_ = 0;

  if (linearClampSampler_ != VK_NULL_HANDLE)
  {
    vkDestroySampler(device_, linearClampSampler_, nullptr);
    linearClampSampler_ = VK_NULL_HANDLE;
  }
  if (linearRepeatSampler_ != VK_NULL_HANDLE)
  {
    vkDestroySampler(device_, linearRepeatSampler_, nullptr);
    linearRepeatSampler_ = VK_NULL_HANDLE;
  }
  if (nearestClampSampler_ != VK_NULL_HANDLE)
  {
    vkDestroySampler(device_, nearestClampSampler_, nullptr);
    nearestClampSampler_ = VK_NULL_HANDLE;
  }
  if (nearestRepeatSampler_ != VK_NULL_HANDLE)
  {
    vkDestroySampler(device_, nearestRepeatSampler_, nullptr);
    nearestRepeatSampler_ = VK_NULL_HANDLE;
  }

  if (descriptorPool_ != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    descriptorPool_ = VK_NULL_HANDLE;
  }
  if (textureSetLayout_ != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorSetLayout(device_, textureSetLayout_, nullptr);
    textureSetLayout_ = VK_NULL_HANDLE;
  }
}

void VulkanBootstrapApp::destroyTextureSlotResources(int index, bool freeDescriptorSet)
{
  if (index < 0 || index >= static_cast<int>(kMaxTextures))
  {
    return;
  }

  TextureSlot &slot = textureSlots_[index];
  if (slot.imageView != VK_NULL_HANDLE)
  {
    vkDestroyImageView(device_, slot.imageView, nullptr);
    slot.imageView = VK_NULL_HANDLE;
  }
  if (slot.image != VK_NULL_HANDLE)
  {
    vkDestroyImage(device_, slot.image, nullptr);
    slot.image = VK_NULL_HANDLE;
  }
  if (slot.memory != VK_NULL_HANDLE)
  {
    vkFreeMemory(device_, slot.memory, nullptr);
    slot.memory = VK_NULL_HANDLE;
  }
  if (freeDescriptorSet && slot.descriptorSet != VK_NULL_HANDLE && descriptorPool_ != VK_NULL_HANDLE)
  {
    vkFreeDescriptorSets(device_, descriptorPool_, 1, &slot.descriptorSet);
    slot.descriptorSet = VK_NULL_HANDLE;
  }
}

void VulkanBootstrapApp::updateTextureDescriptor(int index)
{
  if (index < 0 || index >= static_cast<int>(kMaxTextures))
  {
    return;
  }

  TextureSlot &slot = textureSlots_[index];
  if (slot.descriptorSet == VK_NULL_HANDLE || slot.imageView == VK_NULL_HANDLE)
  {
    return;
  }

  VkDescriptorImageInfo imageInfo {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = slot.imageView;
  imageInfo.sampler = getSamplerForSlot(slot);

  VkWriteDescriptorSet descriptorWrite {};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = slot.descriptorSet;
  descriptorWrite.dstBinding = 0;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(device_, 1, &descriptorWrite, 0, nullptr);
}

VkSampler VulkanBootstrapApp::getSamplerForSlot(const TextureSlot &slot) const
{
  if (slot.linearFiltering)
  {
    return slot.clampAddress ? linearClampSampler_ : linearRepeatSampler_;
  }
  return slot.clampAddress ? nearestClampSampler_ : nearestRepeatSampler_;
}

VkPipeline VulkanBootstrapApp::getPipelineForBatch(const DrawBatch &batch) const
{
  const uint32_t pipelineIndex = getPipelineIndex(
    batch.shaderVariant,
    batch.renderState.blendMode,
    batch.renderState.depthTestEnabled,
    batch.renderState.depthWriteEnabled,
    batch.renderState.cullEnabled,
    batch.renderState.cullClockwise);
  return pipelines_[pipelineIndex];
}

uint32_t VulkanBootstrapApp::getPipelineIndex(
  ShaderVariant variant,
  BlendMode blendMode,
  bool depthTestEnabled,
  bool depthWriteEnabled,
  bool cullEnabled,
  bool cullClockwise) const
{
  return
    static_cast<uint32_t>(variant) * 48u +
    static_cast<uint32_t>(blendMode) * 16u +
    (depthTestEnabled ? 8u : 0u) +
    (depthWriteEnabled ? 4u : 0u) +
    (cullEnabled ? 2u : 0u) +
    (cullClockwise ? 1u : 0u);
}

int VulkanBootstrapApp::allocateTextureSlot()
{
  for (uint32_t index = 0; index < kMaxTextures; ++index)
  {
    TextureSlot &slot = textureSlots_[index];
    if (!slot.allocated)
    {
      slot = TextureSlot {};
      slot.allocated = true;
      return static_cast<int>(index);
    }
  }

  std::fprintf(stderr, "[mce_vulkan_boot] Texture pool exhausted\n");
  return fallbackTextureIndex_;
}

void VulkanBootstrapApp::freeTextureSlot(int index)
{
  if (index < 0 || index >= static_cast<int>(kMaxTextures) || index == fallbackTextureIndex_)
  {
    return;
  }
  if (!textureSlots_[index].allocated)
  {
    return;
  }

  destroyTextureSlotResources(index, true);
  textureSlots_[index] = TextureSlot {};
}

void VulkanBootstrapApp::setCurrentTexture(int index)
{
  if (index < 0)
  {
    boundTextureIndex_ = -1;
    return;
  }

  if (index >= 0 &&
      index < static_cast<int>(kMaxTextures) &&
      textureSlots_[index].allocated)
  {
    boundTextureIndex_ = index;
    return;
  }

  boundTextureIndex_ = fallbackTextureIndex_;
}

int VulkanBootstrapApp::getCurrentTexture() const
{
  return boundTextureIndex_;
}

void VulkanBootstrapApp::setTextureLinearFiltering(int index, bool enabled)
{
  if (index < 0 || index >= static_cast<int>(kMaxTextures) || !textureSlots_[index].allocated)
  {
    return;
  }
  if (textureSlots_[index].linearFiltering == enabled)
  {
    return;
  }

  textureSlots_[index].linearFiltering = enabled;
  updateTextureDescriptor(index);
}

void VulkanBootstrapApp::setTextureClampAddress(int index, bool enabled)
{
  if (index < 0 || index >= static_cast<int>(kMaxTextures) || !textureSlots_[index].allocated)
  {
    return;
  }
  if (textureSlots_[index].clampAddress == enabled)
  {
    return;
  }

  textureSlots_[index].clampAddress = enabled;
  updateTextureDescriptor(index);
}

void VulkanBootstrapApp::uploadTextureData(
  int slotIndex,
  uint32_t width,
  uint32_t height,
  const void *pixelData)
{
  if (slotIndex < 0 || slotIndex >= static_cast<int>(kMaxTextures) || pixelData == nullptr)
  {
    return;
  }

  TextureSlot &slot = textureSlots_[slotIndex];
  if (!slot.allocated)
  {
    return;
  }

  const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;
  ensureStagingBuffer(imageSize);

  void *mappedMemory = nullptr;
  if (vkMapMemory(device_, stagingBufferMemory_, 0, imageSize, 0, &mappedMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("vkMapMemory failed for staging buffer");
  }
  std::memcpy(mappedMemory, pixelData, static_cast<size_t>(imageSize));
  vkUnmapMemory(device_, stagingBufferMemory_);

  destroyTextureSlotResources(slotIndex, false);

  VkImageCreateInfo imageInfo {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device_, &imageInfo, nullptr, &slot.image) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateImage failed for texture");
  }

  VkMemoryRequirements memoryRequirements {};
  vkGetImageMemoryRequirements(device_, slot.image, &memoryRequirements);

  VkMemoryAllocateInfo allocInfo {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memoryRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
    memoryRequirements.memoryTypeBits,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &slot.memory) != VK_SUCCESS)
  {
    throw std::runtime_error("vkAllocateMemory failed for texture");
  }

  vkBindImageMemory(device_, slot.image, slot.memory, 0);

  VkCommandBuffer commandBuffer = beginOneTimeCommands();
  transitionImageLayout(
    commandBuffer,
    slot.image,
    VK_IMAGE_LAYOUT_UNDEFINED,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkBufferImageCopy region {};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageExtent = {width, height, 1};
  vkCmdCopyBufferToImage(
    commandBuffer,
    stagingBuffer_,
    slot.image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region);

  transitionImageLayout(
    commandBuffer,
    slot.image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  endOneTimeCommands(commandBuffer);

  VkImageViewCreateInfo viewInfo {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = slot.image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device_, &viewInfo, nullptr, &slot.imageView) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateImageView failed for texture");
  }

  if (slot.descriptorSet == VK_NULL_HANDLE)
  {
    VkDescriptorSetAllocateInfo allocSetInfo {};
    allocSetInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocSetInfo.descriptorPool = descriptorPool_;
    allocSetInfo.descriptorSetCount = 1;
    allocSetInfo.pSetLayouts = &textureSetLayout_;

    if (vkAllocateDescriptorSets(device_, &allocSetInfo, &slot.descriptorSet) != VK_SUCCESS)
    {
      throw std::runtime_error("vkAllocateDescriptorSets failed for texture");
    }
  }

  slot.width = width;
  slot.height = height;
  updateTextureDescriptor(slotIndex);

#ifdef _DEBUG
  std::fprintf(
    stderr,
    "[mce_vulkan_boot] Uploaded texture slot %d: %ux%u\n",
    slotIndex,
    width,
    height);
#endif
}

void VulkanBootstrapApp::updateTextureData(
  int slotIndex,
  int xOffset,
  int yOffset,
  uint32_t width,
  uint32_t height,
  const void *pixelData)
{
  if (slotIndex < 0 || slotIndex >= static_cast<int>(kMaxTextures) || pixelData == nullptr)
  {
    return;
  }

  TextureSlot &slot = textureSlots_[slotIndex];
  if (!slot.allocated || slot.image == VK_NULL_HANDLE)
  {
    return;
  }

  const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4;
  ensureStagingBuffer(imageSize);

  void *mappedMemory = nullptr;
  if (vkMapMemory(device_, stagingBufferMemory_, 0, imageSize, 0, &mappedMemory) != VK_SUCCESS)
  {
    throw std::runtime_error("vkMapMemory failed for staging update");
  }
  std::memcpy(mappedMemory, pixelData, static_cast<size_t>(imageSize));
  vkUnmapMemory(device_, stagingBufferMemory_);

  VkCommandBuffer commandBuffer = beginOneTimeCommands();
  transitionImageLayout(
    commandBuffer,
    slot.image,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  VkBufferImageCopy region {};
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {xOffset, yOffset, 0};
  region.imageExtent = {width, height, 1};
  vkCmdCopyBufferToImage(
    commandBuffer,
    stagingBuffer_,
    slot.image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &region);

  transitionImageLayout(
    commandBuffer,
    slot.image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  endOneTimeCommands(commandBuffer);
}

VkSurfaceFormatKHR VulkanBootstrapApp::chooseSwapSurfaceFormat(
  const std::vector<VkSurfaceFormatKHR> &availableFormats) const
{
  // Prefer RGBA to match our texture upload format (R8G8B8A8)
  for (const VkSurfaceFormatKHR &format : availableFormats)
  {
    if (format.format == VK_FORMAT_R8G8B8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }
  // Fallback to BGRA (common on MoltenVK/macOS)
  for (const VkSurfaceFormatKHR &format : availableFormats)
  {
    if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }

  return availableFormats.front();
}

VkPresentModeKHR VulkanBootstrapApp::chooseSwapPresentMode(
  const std::vector<VkPresentModeKHR> &availablePresentModes) const
{
  const auto iterator = std::find(
    availablePresentModes.begin(),
    availablePresentModes.end(),
    VK_PRESENT_MODE_MAILBOX_KHR);
  if (iterator != availablePresentModes.end())
  {
    return *iterator;
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanBootstrapApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
  {
    return capabilities.currentExtent;
  }

  int framebufferWidth = 0;
  int framebufferHeight = 0;
  glfwGetFramebufferSize(window_, &framebufferWidth, &framebufferHeight);

  VkExtent2D actualExtent {
    static_cast<uint32_t>(framebufferWidth),
    static_cast<uint32_t>(framebufferHeight)
  };
  actualExtent.width = std::clamp(
    actualExtent.width,
    capabilities.minImageExtent.width,
    capabilities.maxImageExtent.width);
  actualExtent.height = std::clamp(
    actualExtent.height,
    capabilities.minImageExtent.height,
    capabilities.maxImageExtent.height);
  return actualExtent;
}
