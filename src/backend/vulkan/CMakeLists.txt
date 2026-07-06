add_library(
  quark_backend_vulkan STATIC vulkan_context.cpp triangle_vertex_buffer.cpp
                              triangle_renderer.cpp rhi_translation.cpp)
add_library(quark::backend::vulkan ALIAS quark_backend_vulkan)

add_subdirectory(instance)
add_subdirectory(device)
add_subdirectory(frame)
add_subdirectory(presentation)
add_subdirectory(descriptor)
add_subdirectory(pipeline)
add_subdirectory(sync)

target_link_libraries(
  quark_backend_vulkan
  PRIVATE quark::backend::vulkan::instance
          quark::backend::vulkan::device
          quark::backend::vulkan::frame
          quark::backend::vulkan::presentation
          quark::backend::vulkan::descriptor
          quark::backend::vulkan::pipeline
          quark::backend::vulkan::sync
  PUBLIC quark::headers::vk)

target_include_directories(quark_backend_vulkan
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
