add_executable(quark
  main.cpp
  vulkan/VulkanContext.cpp
)

find_package(Vulkan QUIET)
find_package(glfw3 CONFIG QUIET)

if(NOT Vulkan_FOUND)
  message(FATAL_ERROR
    "Vulkan SDK/loader was not found. If you use vcpkg, ensure the vcpkg submodule is initialised and bootstrapped (make deps) and that dependencies from vcpkg.json are installed for the active triplet."
  )
endif()

if(NOT glfw3_FOUND)
  find_package(glfw3 REQUIRED)
endif()

set_target_properties(quark PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_options(quark PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -pedantic-errors>
)

target_include_directories(quark PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(run
  COMMAND $<TARGET_FILE:quark>
  DEPENDS quark
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

quark_apply_sanitizers(quark)
quark_apply_lto(quark)

target_link_libraries(quark PRIVATE Vulkan::Vulkan)

if(TARGET glfw)
  target_link_libraries(quark PRIVATE glfw)
elseif(TARGET glfw::glfw)
  target_link_libraries(quark PRIVATE glfw::glfw)
else()
  message(FATAL_ERROR "GLFW target was not found")
endif()
