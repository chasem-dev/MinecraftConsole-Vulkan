include(FetchContent)

function(mce_collect_vcxproj_sources out_var base_dir vcxproj)
  file(STRINGS "${vcxproj}" vcxproj_lines REGEX "<ClCompile Include=\"[^\"]+\"")
  set(result)
  foreach(line IN LISTS vcxproj_lines)
    string(REGEX REPLACE ".*<ClCompile Include=\"([^\"]+)\".*" "\\1" relative_path "${line}")
    string(REPLACE "\\" "/" relative_path "${relative_path}")

    if(base_dir MATCHES "/Minecraft\\.Client$")
      if(relative_path MATCHES "^(Apple|MacOSX|Vulkan|PS3|PSVita|Orbis|Durango|Xbox|Windows64)/")
        continue()
      endif()
      # Exclude Xbox XUI framework (macOS uses Common/UI/ instead)
      if(relative_path MATCHES "^Common/XUI/")
        continue()
      endif()
      # Exclude Sony network code
      if(relative_path MATCHES "^Common/Network/Sony/")
        continue()
      endif()
      if(relative_path MATCHES "^KeyboardMouseInput\\.cpp$")
        continue()
      endif()
    endif()

    if(base_dir MATCHES "/Minecraft\\.World$")
      # Exclude files with missing headers on macOS
      if(relative_path MATCHES "^(SkyIslandDimension|Hasher|DurangoStats|MemoryLevelStorage|MemoryLevelStorageSource)\\.cpp$")
        continue()
      endif()
    endif()

    list(APPEND result "${base_dir}/${relative_path}")
  endforeach()

  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

find_package(Vulkan QUIET)
if(NOT Vulkan_FOUND)
  message(FATAL_ERROR
    "Vulkan SDK not found. Install the macOS Vulkan SDK with MoltenVK and make it visible "
    "to CMake before configuring this target.")
endif()
find_package(glfw3 CONFIG QUIET)
find_program(GLSLANG_VALIDATOR NAMES glslangValidator HINTS /opt/homebrew/bin REQUIRED)

if(TARGET glfw)
  set(MCE_GLFW_TARGET glfw)
elseif(TARGET glfw3)
  set(MCE_GLFW_TARGET glfw3)
else()
  FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
  )
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(glfw)
  set(MCE_GLFW_TARGET glfw)
endif()

add_executable(mce_vulkan_boot MACOSX_BUNDLE
  "main.cpp"
  "../Apple/AppleStubs.cpp"
  "../Apple/AppleInputStubs.cpp"
  "../Apple/AppleLinkerStubs.cpp"
  "../Vulkan/4JLibs/inc/4J_Render.h"
  "../Vulkan/VulkanBootstrapApp.h"
  "../Vulkan/VulkanBootstrapAppCore.cpp"
  "../Vulkan/VulkanBootstrapAppRender.cpp"
  "../Vulkan/VulkanRenderManager.cpp"
  $<TARGET_OBJECTS:mce_core_probe>
  $<TARGET_OBJECTS:mce_client_legacy>
  $<TARGET_OBJECTS:mce_world_legacy>
)

add_library(mce_core_probe OBJECT EXCLUDE_FROM_ALL
  "../Apple/Apple_App.cpp"
  "../Vulkan/Vulkan_UIController.cpp"
)

mce_collect_vcxproj_sources(
  MCE_CLIENT_LEGACY_SOURCES
  "${CMAKE_SOURCE_DIR}/Minecraft.Client"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/Minecraft.Client.vcxproj"
)

mce_collect_vcxproj_sources(
  MCE_WORLD_LEGACY_SOURCES
  "${CMAKE_SOURCE_DIR}/Minecraft.World"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/Minecraft.World.vcxproj"
)

add_library(mce_client_legacy OBJECT EXCLUDE_FROM_ALL
  ${MCE_CLIENT_LEGACY_SOURCES}
)

add_library(mce_world_legacy OBJECT EXCLUDE_FROM_ALL
  ${MCE_WORLD_LEGACY_SOURCES}
)

set(MCE_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
set(MCE_VERTEX_SHADER "${MCE_SHADER_OUTPUT_DIR}/bootstrap_triangle.vert.spv")
set(MCE_FRAGMENT_SHADER "${MCE_SHADER_OUTPUT_DIR}/bootstrap_triangle.frag.spv")

add_custom_command(
  OUTPUT "${MCE_VERTEX_SHADER}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${MCE_SHADER_OUTPUT_DIR}"
  COMMAND "${GLSLANG_VALIDATOR}" -V -S vert
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/bootstrap_triangle.vert"
    -o "${MCE_VERTEX_SHADER}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/bootstrap_triangle.vert"
  COMMENT "Compiling bootstrap vertex shader"
  VERBATIM
)

add_custom_command(
  OUTPUT "${MCE_FRAGMENT_SHADER}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${MCE_SHADER_OUTPUT_DIR}"
  COMMAND "${GLSLANG_VALIDATOR}" -V -S frag
    "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/bootstrap_triangle.frag"
    -o "${MCE_FRAGMENT_SHADER}"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Shaders/bootstrap_triangle.frag"
  COMMENT "Compiling bootstrap fragment shader"
  VERBATIM
)

add_custom_target(
  mce_vulkan_boot_shaders
  DEPENDS "${MCE_VERTEX_SHADER}" "${MCE_FRAGMENT_SHADER}"
)

add_dependencies(mce_vulkan_boot mce_vulkan_boot_shaders)

target_compile_features(mce_vulkan_boot PRIVATE cxx_std_17)
target_compile_features(mce_core_probe PRIVATE cxx_std_17)
target_compile_features(mce_client_legacy PRIVATE cxx_std_17)
target_compile_features(mce_world_legacy PRIVATE cxx_std_17)

# Apple/macOS porting: relax warnings for legacy code
set(MCE_LEGACY_FLAGS
  -fdeclspec
  -Wno-nonportable-include-path
  -Wno-invalid-source-encoding
  -Wno-reserved-user-defined-literal
  -Wno-non-c-typedef-for-linkage
  -Wno-#pragma-messages
  -Wno-reorder-ctor
  -Wno-delete-incomplete
  -Wno-switch
  -Wno-unused-value
  -Wno-unused-variable
  -Wno-unused-private-field
  -Wno-writable-strings
  -Wno-tautological-constant-out-of-range-compare
  -Wno-missing-braces
  -Wno-sometimes-uninitialized
  -Wno-format-security
  -Wno-deprecated-declarations
  -ferror-limit=0
)
target_compile_options(mce_vulkan_boot PRIVATE ${MCE_LEGACY_FLAGS})
target_compile_options(mce_core_probe PRIVATE ${MCE_LEGACY_FLAGS})
target_compile_options(mce_client_legacy PRIVATE ${MCE_LEGACY_FLAGS})
target_compile_options(mce_world_legacy PRIVATE ${MCE_LEGACY_FLAGS})
target_include_directories(mce_vulkan_boot PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  "${CMAKE_SOURCE_DIR}/Minecraft.Client"
  "${CMAKE_SOURCE_DIR}/Minecraft.World"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/x64headers"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/PS3/PS3Extras/DirectX"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkan"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkan/4JLibs/inc"
)
target_include_directories(mce_core_probe PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  "${CMAKE_SOURCE_DIR}/Minecraft.World/x64headers"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/PS3/PS3Extras/DirectX"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkan"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkan/4JLibs/inc"
)
target_include_directories(mce_client_legacy PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  "${CMAKE_SOURCE_DIR}/Minecraft.Client"
  "${CMAKE_SOURCE_DIR}/Minecraft.World"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/x64headers"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/PS3/PS3Extras/DirectX"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkan"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkan/4JLibs/inc"
)
target_include_directories(mce_world_legacy PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  "${CMAKE_SOURCE_DIR}/Minecraft.Client"
  "${CMAKE_SOURCE_DIR}/Minecraft.World"
  "${CMAKE_SOURCE_DIR}/Minecraft.World/x64headers"
  "${CMAKE_SOURCE_DIR}/Minecraft.Client/PS3/PS3Extras/DirectX"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkan"
  "${CMAKE_CURRENT_SOURCE_DIR}/../Vulkan/4JLibs/inc"
)
target_compile_definitions(mce_vulkan_boot PRIVATE
  MCE_BOOT_VERTEX_SHADER_PATH="${MCE_VERTEX_SHADER}"
  MCE_BOOT_FRAGMENT_SHADER_PATH="${MCE_FRAGMENT_SHADER}"
  MCE_VULKAN_VALIDATION
)
target_compile_definitions(mce_core_probe PRIVATE
  MCE_BOOT_VERTEX_SHADER_PATH="${MCE_VERTEX_SHADER}"
  MCE_BOOT_FRAGMENT_SHADER_PATH="${MCE_FRAGMENT_SHADER}"
  MCE_VULKAN_VALIDATION
)
target_compile_definitions(mce_client_legacy PRIVATE
  MCE_BOOT_VERTEX_SHADER_PATH="${MCE_VERTEX_SHADER}"
  MCE_BOOT_FRAGMENT_SHADER_PATH="${MCE_FRAGMENT_SHADER}"
  MCE_VULKAN_VALIDATION
)
target_compile_definitions(mce_world_legacy PRIVATE
  MCE_BOOT_VERTEX_SHADER_PATH="${MCE_VERTEX_SHADER}"
  MCE_BOOT_FRAGMENT_SHADER_PATH="${MCE_FRAGMENT_SHADER}"
  MCE_VULKAN_VALIDATION
)
target_link_libraries(mce_vulkan_boot PRIVATE Vulkan::Vulkan ${MCE_GLFW_TARGET})
target_link_libraries(mce_core_probe PRIVATE Vulkan::Vulkan ${MCE_GLFW_TARGET})
target_link_libraries(mce_client_legacy PRIVATE Vulkan::Vulkan ${MCE_GLFW_TARGET})
target_link_libraries(mce_world_legacy PRIVATE Vulkan::Vulkan ${MCE_GLFW_TARGET})

if(APPLE)
  target_link_libraries(mce_vulkan_boot PRIVATE
    "-framework Cocoa"
    "-framework IOKit"
    "-framework CoreVideo"
    "-framework QuartzCore"
    "-framework Metal"
  )
endif()
