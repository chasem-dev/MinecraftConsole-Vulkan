cmake_minimum_required(VERSION 3.24)

project(MinecraftCommunityEdition LANGUAGES C CXX)

option(MCE_BUILD_MACOS_BOOT "Build the macOS Vulkan bootstrap target" ON)

if(APPLE AND MCE_BUILD_MACOS_BOOT)
  add_subdirectory("Minecraft.Client/MacOSX")
else()
  message(STATUS "Skipping macOS bootstrap target.")
endif()
