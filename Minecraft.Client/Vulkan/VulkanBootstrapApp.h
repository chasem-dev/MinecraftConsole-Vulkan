#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

#ifndef VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR
#define VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR 0x00000001
#endif

class VulkanBootstrapApp
{
public:
  enum class ShaderVariant : uint32_t
  {
    ColorOnly = 0,
    Textured,
    TexturedAlphaTest,
    TexturedFog,
    TexturedFogAlphaTest,
    Count
  };

  enum class BlendMode : uint32_t
  {
    Opaque = 0,
    Alpha,
    Additive,
    Count
  };

  struct Vertex
  {
    float position[3];
    float texCoord[2];
    float color[4];
  };

  struct RenderState
  {
    BlendMode blendMode = BlendMode::Opaque;
    bool depthTestEnabled = true;
    bool depthWriteEnabled = true;
    bool cullEnabled = false;
    bool cullClockwise = true;
    int textureIndex = -1;
  };

  struct TextureSlot
  {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    bool allocated = false;
    bool linearFiltering = false;
    bool clampAddress = false;
  };

  static constexpr uint32_t kMaxTextures = 256;

  static void initialiseGlfw();
  static void terminateGlfw();
  static GLFWwindow *createWindow(const char *title, uint32_t width = 1280, uint32_t height = 720);

  void run();
  void attachToWindow(GLFWwindow *window);
  void beginFrame();
  void submitVertices(
    VkPrimitiveTopology topology,
    const Vertex *vertices,
    size_t count,
    ShaderVariant variant,
    const float mvp[16],
    const RenderState &state);
  void requestClear(uint32_t flags);
  void tickFrame();
  void shutdownRenderer();
  void setClearColour(const float colourRGBA[4]);
  void setViewportRect(int x, int y, uint32_t width, uint32_t height);
  int allocateTextureSlot();
  void freeTextureSlot(int index);
  void setCurrentTexture(int index);
  int getCurrentTexture() const;
  void setTextureLinearFiltering(int index, bool enabled);
  void setTextureClampAddress(int index, bool enabled);
  void uploadTextureData(int slotIndex, uint32_t width, uint32_t height, const void *pixelData);
  void updateTextureData(
    int slotIndex,
    int xOffset,
    int yOffset,
    uint32_t width,
    uint32_t height,
    const void *pixelData);

private:
  struct DrawBatch
  {
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    uint32_t firstVertex = 0;
    uint32_t vertexCount = 0;
    uint32_t clearFlags = 0;
    ShaderVariant shaderVariant = ShaderVariant::ColorOnly;
    RenderState renderState {};
    std::array<float, 16> mvp {};
    int viewportX = 0;
    int viewportY = 0;
    uint32_t viewportWidth = 0;
    uint32_t viewportHeight = 0;
  };

  struct QueueFamilyIndices
  {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const;
  };

  struct SwapchainSupportDetails
  {
    VkSurfaceCapabilitiesKHR capabilities {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
  };

  void initWindow();
  void initVulkan();
  void mainLoop();
  void cleanup();
  void recreateSwapchain();
  void cleanupSwapchain();

  void createInstance();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createCommandPool();
  void createSwapchain();
  void createImageViews();
  void createDepthResources();
  void createRenderPass();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createVertexBuffer(size_t vertexCapacity);
  void ensureVertexBufferCapacity(size_t requiredVertices);
  void createCommandBuffers();
  void createSyncObjects();
  void createTextureResources();
  void createDescriptorPool();
  void createDescriptorSetLayout();
  void createSamplers();
  void createFallbackTexture();
  void drawFrame();
  void recordCommandBuffer(
    VkCommandBuffer commandBuffer,
    uint32_t imageIndex,
    const std::vector<DrawBatch> &batches);
  uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
  void createBuffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer &buffer,
    VkDeviceMemory &bufferMemory);
  void copyBuffer(VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size);
  VkCommandBuffer beginOneTimeCommands();
  void endOneTimeCommands(VkCommandBuffer commandBuffer);
  void transitionImageLayout(
    VkCommandBuffer commandBuffer,
    VkImage image,
    VkImageLayout oldLayout,
    VkImageLayout newLayout);
  void ensureStagingBuffer(VkDeviceSize requiredSize);
  void destroyTextureResources();
  void destroyTextureSlotResources(int index, bool freeDescriptorSet);
  void updateTextureDescriptor(int index);
  VkSampler getSamplerForSlot(const TextureSlot &slot) const;
  VkPipeline getPipelineForBatch(const DrawBatch &batch) const;
  uint32_t getPipelineIndex(
    ShaderVariant variant,
    BlendMode blendMode,
    bool depthTestEnabled,
    bool depthWriteEnabled,
    bool cullEnabled,
    bool cullClockwise) const;
  void logSelectedDevice() const;
  void logSwapchainSelection(
    VkSurfaceFormatKHR surfaceFormat,
    VkPresentModeKHR presentMode,
    VkExtent2D extent) const;

  bool isDeviceSuitable(VkPhysicalDevice device) const;
  bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
  SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device) const;

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) const;
  VkPresentModeKHR chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) const;
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const;

  std::vector<const char *> getRequiredInstanceExtensions() const;
  std::vector<const char *> getRequiredDeviceExtensions() const;

  static void framebufferResizeCallback(GLFWwindow *window, int width, int height);

  static constexpr uint32_t kWindowWidth = 1280;
  static constexpr uint32_t kWindowHeight = 720;

  GLFWwindow *window_ = nullptr;

  VkInstance instance_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
  VkDevice device_ = VK_NULL_HANDLE;
  VkQueue graphicsQueue_ = VK_NULL_HANDLE;
  VkQueue presentQueue_ = VK_NULL_HANDLE;
  VkCommandPool commandPool_ = VK_NULL_HANDLE;

  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  std::vector<VkImage> swapchainImages_;
  std::vector<VkImageView> swapchainImageViews_;
  std::vector<VkFramebuffer> swapchainFramebuffers_;
  VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
  VkExtent2D swapchainExtent_ {};
  VkImage depthImage_ = VK_NULL_HANDLE;
  VkDeviceMemory depthImageMemory_ = VK_NULL_HANDLE;
  VkImageView depthImageView_ = VK_NULL_HANDLE;
  VkRenderPass renderPass_ = VK_NULL_HANDLE;
  VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
  std::array<VkPipeline, 240> pipelines_ {};
  std::vector<VkCommandBuffer> commandBuffers_;

  VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
  VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
  void *vertexBufferMapped_ = nullptr;
  size_t vertexBufferCapacity_ = 0;
  std::mutex frameDataMutex_;
  int currentViewportX_ = 0;
  int currentViewportY_ = 0;
  uint32_t currentViewportWidth_ = 0;
  uint32_t currentViewportHeight_ = 0;
  std::vector<Vertex> frameVertices_;
  std::vector<DrawBatch> frameBatches_;

  VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
  VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;
  VkFence inFlightFence_ = VK_NULL_HANDLE;

  TextureSlot textureSlots_[kMaxTextures] {};
  VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
  VkDescriptorSetLayout textureSetLayout_ = VK_NULL_HANDLE;
  VkSampler nearestRepeatSampler_ = VK_NULL_HANDLE;
  VkSampler nearestClampSampler_ = VK_NULL_HANDLE;
  VkSampler linearRepeatSampler_ = VK_NULL_HANDLE;
  VkSampler linearClampSampler_ = VK_NULL_HANDLE;
  VkBuffer stagingBuffer_ = VK_NULL_HANDLE;
  VkDeviceMemory stagingBufferMemory_ = VK_NULL_HANDLE;
  VkDeviceSize stagingBufferSize_ = 0;
  int boundTextureIndex_ = -1;
  int fallbackTextureIndex_ = -1;

  bool framebufferResized_ = false;
  bool startupInfoLogged_ = false;
  float clearColour_[4] = {0.05f, 0.06f, 0.09f, 1.0f};
};
