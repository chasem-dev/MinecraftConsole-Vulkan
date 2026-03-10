# Vulkan Bootstrap On macOS

This target is the first Vulkan bring-up step for the project. The macOS host app lives in this folder, while the reusable Vulkan implementation now lives under `Minecraft.Client/Vulkan/`.

Current scope:
- Creates a macOS app target with CMake
- Opens a GLFW window
- Initializes the Vulkan backend on macOS through MoltenVK
- Creates a swapchain and presents a clear color

This is not the full game yet. It is only the boot-first platform target needed before integrating the existing renderer and game systems.

## Prerequisites

1. Install a macOS Vulkan SDK that includes MoltenVK and exposes `vulkan/vulkan.h` plus the Vulkan loader to CMake.
2. Make sure CMake can find that SDK before configuring this project.

One workable Homebrew-based setup is:

```sh
brew install molten-vk vulkan-loader vulkan-headers glslang
```

The bootstrap target fetches GLFW automatically if it is not already installed on the machine.

## Configure

```sh
cmake -S . -B build-macos
```

## Build

```sh
cmake --build build-macos --target mce_vulkan_boot --parallel 10
```

## Run

```sh
open build-macos/Minecraft.Client/MacOSX/mce_vulkan_boot.app
```
