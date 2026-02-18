add_library(quark_backend_vulkan_device STATIC
  device.cpp
  device_registry.cpp
  device_bundle.cpp
)
add_library(quark::backend::vulkan::device ALIAS quark_backend_vulkan_device)

target_link_libraries(quark_backend_vulkan_device
  PUBLIC
    quark::headers
  PRIVATE
    Vulkan::Vulkan
)

target_include_directories(quark_backend_vulkan_device
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
