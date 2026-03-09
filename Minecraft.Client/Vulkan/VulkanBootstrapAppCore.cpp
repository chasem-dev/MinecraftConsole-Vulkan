#include "VulkanBootstrapApp.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr const char *kPortabilityEnumerationExtension = "VK_KHR_portability_enumeration";
constexpr const char *kGetPhysicalDeviceProperties2Extension = "VK_KHR_get_physical_device_properties2";
constexpr const char *kPortabilitySubsetExtension = "VK_KHR_portability_subset";

std::string getGlfwErrorMessage()
{
  const char *description = nullptr;
  const int errorCode = glfwGetError(&description);
  if (errorCode == GLFW_NO_ERROR)
  {
    return "no GLFW error";
  }

  std::string message = "GLFW error ";
  message += std::to_string(errorCode);
  if (description != nullptr)
  {
    message += ": ";
    message += description;
  }
  return message;
}
}

bool VulkanBootstrapApp::QueueFamilyIndices::isComplete() const
{
  return graphicsFamily.has_value() && presentFamily.has_value();
}

void VulkanBootstrapApp::initialiseGlfw()
{
  glfwInitVulkanLoader(vkGetInstanceProcAddr);

  if (glfwInit() != GLFW_TRUE)
  {
    throw std::runtime_error("glfwInit failed: " + getGlfwErrorMessage());
  }

  if (glfwVulkanSupported() != GLFW_TRUE)
  {
    throw std::runtime_error("GLFW could not find a usable Vulkan loader or ICD: " + getGlfwErrorMessage());
  }
}

void VulkanBootstrapApp::terminateGlfw()
{
  glfwTerminate();
}

GLFWwindow *VulkanBootstrapApp::createWindow(const char *title, uint32_t width, uint32_t height)
{
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  GLFWwindow *window = glfwCreateWindow(
    static_cast<int>(width),
    static_cast<int>(height),
    title,
    nullptr,
    nullptr);
  if (window == nullptr)
  {
    throw std::runtime_error("glfwCreateWindow failed: " + getGlfwErrorMessage());
  }

  return window;
}

void VulkanBootstrapApp::run()
{
  initialiseGlfw();
  window_ = createWindow("Minecraft Community Edition Vulkan bootstrap");
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
  initVulkan();
  mainLoop();
  shutdownRenderer();
  glfwDestroyWindow(window_);
  window_ = nullptr;
  terminateGlfw();
}

void VulkanBootstrapApp::attachToWindow(GLFWwindow *window)
{
  window_ = window;
  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
  createInstance();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createCommandPool();
  createSwapchain();
  createImageViews();
  createDepthResources();
  createTextureResources();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createVertexBuffer(262144);
  createCommandBuffers();
  createSyncObjects();
  setViewportRect(0, 0, swapchainExtent_.width, swapchainExtent_.height);
}

void VulkanBootstrapApp::initVulkan()
{
  attachToWindow(window_);
}

void VulkanBootstrapApp::mainLoop()
{
  while (!glfwWindowShouldClose(window_))
  {
    glfwPollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(device_);
}

void VulkanBootstrapApp::tickFrame()
{
  drawFrame();
}

void VulkanBootstrapApp::beginFrame()
{
  std::lock_guard<std::mutex> lock(frameDataMutex_);
  frameVertices_.clear();
  frameBatches_.clear();
}

void VulkanBootstrapApp::submitVertices(
  VkPrimitiveTopology topology,
  const Vertex *vertices,
  size_t count,
  ShaderVariant variant,
  const float mvp[16],
  const RenderState &state)
{
  if (vertices == nullptr || count == 0)
  {
    return;
  }

  std::lock_guard<std::mutex> lock(frameDataMutex_);
  DrawBatch batch;
  batch.topology = topology;
  batch.firstVertex = static_cast<uint32_t>(frameVertices_.size());
  batch.vertexCount = static_cast<uint32_t>(count);
  batch.shaderVariant = variant;
  batch.renderState = state;
  batch.viewportX = currentViewportX_;
  batch.viewportY = currentViewportY_;
  batch.viewportWidth = currentViewportWidth_;
  batch.viewportHeight = currentViewportHeight_;
  if (mvp != nullptr)
  {
    std::copy(mvp, mvp + batch.mvp.size(), batch.mvp.begin());
  }
  frameBatches_.push_back(batch);
  frameVertices_.insert(frameVertices_.end(), vertices, vertices + count);
}

void VulkanBootstrapApp::requestClear(uint32_t flags)
{
  std::lock_guard<std::mutex> lock(frameDataMutex_);
  DrawBatch batch;
  batch.clearFlags = flags;
  batch.viewportX = currentViewportX_;
  batch.viewportY = currentViewportY_;
  batch.viewportWidth = currentViewportWidth_;
  batch.viewportHeight = currentViewportHeight_;
  frameBatches_.push_back(batch);
}

void VulkanBootstrapApp::setViewportRect(int x, int y, uint32_t width, uint32_t height)
{
  std::lock_guard<std::mutex> lock(frameDataMutex_);
  currentViewportX_ = x;
  currentViewportY_ = y;
  currentViewportWidth_ = width;
  currentViewportHeight_ = height;
}

void VulkanBootstrapApp::shutdownRenderer()
{
  cleanupSwapchain();
  destroyTextureResources();

  if (imageAvailableSemaphore_ != VK_NULL_HANDLE)
  {
    vkDestroySemaphore(device_, imageAvailableSemaphore_, nullptr);
    imageAvailableSemaphore_ = VK_NULL_HANDLE;
  }
  if (renderFinishedSemaphore_ != VK_NULL_HANDLE)
  {
    vkDestroySemaphore(device_, renderFinishedSemaphore_, nullptr);
    renderFinishedSemaphore_ = VK_NULL_HANDLE;
  }
  if (inFlightFence_ != VK_NULL_HANDLE)
  {
    vkDestroyFence(device_, inFlightFence_, nullptr);
    inFlightFence_ = VK_NULL_HANDLE;
  }
  if (commandPool_ != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(device_, commandPool_, nullptr);
    commandPool_ = VK_NULL_HANDLE;
  }
  if (vertexBuffer_ != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(device_, vertexBuffer_, nullptr);
    vertexBuffer_ = VK_NULL_HANDLE;
  }
  if (vertexBufferMapped_ != nullptr)
  {
    vkUnmapMemory(device_, vertexBufferMemory_);
    vertexBufferMapped_ = nullptr;
  }
  if (vertexBufferMemory_ != VK_NULL_HANDLE)
  {
    vkFreeMemory(device_, vertexBufferMemory_, nullptr);
    vertexBufferMemory_ = VK_NULL_HANDLE;
  }
  if (device_ != VK_NULL_HANDLE)
  {
    vkDestroyDevice(device_, nullptr);
    device_ = VK_NULL_HANDLE;
  }
  if (surface_ != VK_NULL_HANDLE)
  {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    surface_ = VK_NULL_HANDLE;
  }
  if (instance_ != VK_NULL_HANDLE)
  {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
}

void VulkanBootstrapApp::cleanup()
{
  shutdownRenderer();
  if (window_ != nullptr)
  {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  terminateGlfw();
}

void VulkanBootstrapApp::createInstance()
{
  VkApplicationInfo appInfo {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Minecraft Community Edition Vulkan bootstrap";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName = "MCE bootstrap";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_API_VERSION_1_1;

  const auto requiredExtensions = getRequiredInstanceExtensions();

  VkInstanceCreateInfo createInfo {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
  createInfo.ppEnabledExtensionNames = requiredExtensions.data();
#if defined(__APPLE__)
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

#ifdef MCE_VULKAN_VALIDATION
  {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    bool validationAvailable = false;
    for (const auto& layer : availableLayers)
    {
      if (std::string(layer.layerName) == "VK_LAYER_KHRONOS_validation")
      {
        validationAvailable = true;
        break;
      }
    }

    if (validationAvailable)
    {
      static const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};
      createInfo.enabledLayerCount = 1;
      createInfo.ppEnabledLayerNames = validationLayers;
      std::cout << "[mce_vulkan_boot] Vulkan validation layers enabled\n";
    }
    else
    {
      std::cout << "[mce_vulkan_boot] Validation layers requested but VK_LAYER_KHRONOS_validation not available\n";
    }
  }
#endif

  if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateInstance failed");
  }
}

void VulkanBootstrapApp::createSurface()
{
  const VkResult result = glfwCreateWindowSurface(instance_, window_, nullptr, &surface_);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error(
      "glfwCreateWindowSurface failed with VkResult " + std::to_string(static_cast<int>(result)) +
      " (" + getGlfwErrorMessage() + ")");
  }
}

void VulkanBootstrapApp::pickPhysicalDevice()
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
  if (deviceCount == 0)
  {
    throw std::runtime_error("No Vulkan physical devices were found");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

  const auto iterator = std::find_if(devices.begin(), devices.end(), [&](VkPhysicalDevice device) {
    return isDeviceSuitable(device);
  });
  if (iterator == devices.end())
  {
    throw std::runtime_error("No Vulkan device supports graphics, present, and swapchain requirements");
  }

  physicalDevice_ = *iterator;
  logSelectedDevice();
}

void VulkanBootstrapApp::createLogicalDevice()
{
  const QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);
  const std::set<uint32_t> uniqueQueueFamilies {indices.graphicsFamily.value(), indices.presentFamily.value()};

  float queuePriority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  for (uint32_t familyIndex : uniqueQueueFamilies)
  {
    VkDeviceQueueCreateInfo queueCreateInfo {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = familyIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures {};
  const auto requiredExtensions = getRequiredDeviceExtensions();

  VkDeviceCreateInfo createInfo {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
  createInfo.ppEnabledExtensionNames = requiredExtensions.data();

  if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateDevice failed");
  }

  vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
  vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);
}

void VulkanBootstrapApp::createCommandPool()
{
  const QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);

  VkCommandPoolCreateInfo poolInfo {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

  if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS)
  {
    throw std::runtime_error("vkCreateCommandPool failed");
  }
}

bool VulkanBootstrapApp::isDeviceSuitable(VkPhysicalDevice device) const
{
  const QueueFamilyIndices indices = findQueueFamilies(device);
  if (!indices.isComplete() || !checkDeviceExtensionSupport(device))
  {
    return false;
  }

  const SwapchainSupportDetails swapchainSupport = querySwapchainSupport(device);
  return !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
}

bool VulkanBootstrapApp::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  const auto requiredDeviceExtensions = getRequiredDeviceExtensions();
  std::set<std::string> requiredExtensions(
    requiredDeviceExtensions.begin(),
    requiredDeviceExtensions.end());
  for (const VkExtensionProperties &extension : availableExtensions)
  {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

VulkanBootstrapApp::QueueFamilyIndices
VulkanBootstrapApp::findQueueFamilies(VkPhysicalDevice device) const
{
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  for (uint32_t familyIndex = 0; familyIndex < queueFamilyCount; ++familyIndex)
  {
    if ((queueFamilies[familyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
    {
      indices.graphicsFamily = familyIndex;
    }

    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, familyIndex, surface_, &presentSupport);
    if (presentSupport == VK_TRUE)
    {
      indices.presentFamily = familyIndex;
    }

    if (indices.isComplete())
    {
      break;
    }
  }

  return indices;
}

VulkanBootstrapApp::SwapchainSupportDetails
VulkanBootstrapApp::querySwapchainSupport(VkPhysicalDevice device) const
{
  SwapchainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
  if (formatCount > 0)
  {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
  if (presentModeCount > 0)
  {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
      device,
      surface_,
      &presentModeCount,
      details.presentModes.data());
  }

  return details;
}

std::vector<const char *> VulkanBootstrapApp::getRequiredInstanceExtensions() const
{
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  if (glfwExtensions == nullptr)
  {
    throw std::runtime_error(
      "glfwGetRequiredInstanceExtensions failed: " + getGlfwErrorMessage());
  }
  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
#if defined(__APPLE__)
  extensions.push_back(kPortabilityEnumerationExtension);
  extensions.push_back(kGetPhysicalDeviceProperties2Extension);
#endif
#ifdef MCE_VULKAN_VALIDATION
  {
    uint32_t extCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr);
    std::vector<VkExtensionProperties> availableExts(extCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extCount, availableExts.data());
    for (const auto& ext : availableExts)
    {
      if (std::string(ext.extensionName) == VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
      {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        break;
      }
    }
  }
#endif
  return extensions;
}

std::vector<const char *> VulkanBootstrapApp::getRequiredDeviceExtensions() const
{
  std::vector<const char *> extensions {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
#if defined(__APPLE__)
  extensions.push_back(kPortabilitySubsetExtension);
#endif
  return extensions;
}

void VulkanBootstrapApp::framebufferResizeCallback(GLFWwindow *window, int, int)
{
  auto *app = static_cast<VulkanBootstrapApp *>(glfwGetWindowUserPointer(window));
  if (app != nullptr)
  {
    app->framebufferResized_ = true;
  }
}

void VulkanBootstrapApp::logSelectedDevice() const
{
  VkPhysicalDeviceProperties properties {};
  vkGetPhysicalDeviceProperties(physicalDevice_, &properties);

  std::cout
    << "[mce_vulkan_boot] Using Vulkan device: " << properties.deviceName
    << " | API " << VK_VERSION_MAJOR(properties.apiVersion)
    << "." << VK_VERSION_MINOR(properties.apiVersion)
    << "." << VK_VERSION_PATCH(properties.apiVersion)
    << '\n';
}

void VulkanBootstrapApp::logSwapchainSelection(
  VkSurfaceFormatKHR surfaceFormat,
  VkPresentModeKHR presentMode,
  VkExtent2D extent) const
{
  std::cout
    << "[mce_vulkan_boot] Swapchain format=" << surfaceFormat.format
    << " colorspace=" << surfaceFormat.colorSpace
    << " presentMode=" << presentMode
    << " extent=" << extent.width << "x" << extent.height
    << '\n';
}

void VulkanBootstrapApp::setClearColour(const float colourRGBA[4])
{
  std::copy(colourRGBA, colourRGBA + 4, clearColour_);
}
