add_library(quark_backend_vulkan STATIC 
  vulkan_context.cpp
)
add_library(quark::backend::vulkan ALIAS quark_backend_vulkan)

add_subdirectory(instance)

target_link_libraries(quark_backend_vulkan
  INTERFACE
    quark::backend::vulkan::instance
  PUBLIC 
    quark::headers
    Vulkan::Vulkan
)

target_include_directories(quark_backend_vulkan
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
