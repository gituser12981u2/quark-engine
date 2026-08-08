cmake_minimum_required(VERSION 3.20)
project(Quark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(QuarkOptions)
include(QuarkFeatures)

find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
  message(
    FATAL_ERROR
      "Vulkan SDK/loader was not found. If you use vcpkg, ensure the vcpkg submodule is initialised and bootstrapped (make deps) and that dependencies from vcpkg.json are installed for the active triplet."
  )
endif()

find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(tl-expected CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

if(NOT QUARK_HEADLESS)
  find_package(glfw3 CONFIG REQUIRED)
endif()

add_subdirectory(src)
