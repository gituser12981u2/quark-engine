add_library(quark_backend_vulkan_sync STATIC
  gpu_timeline.cpp

)
add_library(quark::backend::vulkan::sync ALIAS quark_backend_vulkan_sync)

target_link_libraries(quark_backend_vulkan_sync
  PUBLIC
    quark::headers::vk
)

target_include_directories(quark_backend_vulkan_sync
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
