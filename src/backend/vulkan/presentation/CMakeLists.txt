add_library(quark_backend_vulkan_presentation STATIC
  presenter_bundle.cpp
  swapchain.cpp
)
add_library(quark::backend::vulkan::presentation ALIAS quark_backend_vulkan_presentation)

target_link_libraries(quark_backend_vulkan_presentation
  PUBLIC
    quark::headers::vk
  PRIVATE
    Vulkan::Vulkan
)

target_include_directories(quark_backend_vulkan_presentation
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
