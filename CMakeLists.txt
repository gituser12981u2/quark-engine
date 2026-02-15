cmake_minimum_required(VERSION 3.20)
project(Quark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(QuarkOptions)

find_package(Vulkan QUIET)
# find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG QUIET)

if(NOT Vulkan_FOUND)
  message(FATAL_ERROR
    "Vulkan SDK/loader was not found. If you use vcpkg, ensure the vcpkg submodule is initialised and bootstrapped (make deps) and that dependencies from vcpkg.json are installed for the active triplet."
  )
endif()

if(NOT glfw3_FOUND)
  find_package(glfw3 REQUIRED)
endif()

add_subdirectory(src)
