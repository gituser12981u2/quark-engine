add_library(
  quark_backend_vulkan_pipeline STATIC
  graphics_pipeline_bundle.cpp graphics_pipeline_registry.cpp
  graphics_pipeline.cpp pipeline.cpp pipeline_layout.cpp)
add_library(quark::backend::vulkan::pipeline ALIAS
            quark_backend_vulkan_pipeline)

target_link_libraries(quark_backend_vulkan_pipeline PUBLIC quark::headers::vk)

target_include_directories(quark_backend_vulkan_pipeline
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
