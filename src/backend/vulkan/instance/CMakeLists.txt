add_library(quark_backend_vulkan_instance STATIC 
  debug_messenger.cpp
  instance.cpp
  instance_registry.cpp
  instance_bundle.cpp
)
add_library(quark::backend::vulkan::instance ALIAS quark_backend_vulkan_instance)

target_link_libraries(quark_backend_vulkan_instance
  PUBLIC
    quark::headers
  PRIVATE 
    Vulkan::Vulkan
)

target_include_directories(quark_backend_vulkan_instance
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
