add_library(quark_backend_vulkan_frame STATIC
  frame_cmd.cpp
  frame_sync.cpp
)
add_library(quark::backend::vulkan::frame ALIAS quark_backend_vulkan_frame)

target_link_libraries(quark_backend_vulkan_frame
  PUBLIC
    quark::headers::vk
)

target_include_directories(quark_backend_vulkan_frame
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
