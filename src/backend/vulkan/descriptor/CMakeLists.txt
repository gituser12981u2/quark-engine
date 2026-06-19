add_library(quark_backend_vulkan_descriptor STATIC
            descriptor_set_layout.cpp descriptor_set_layout_registry.cpp)
add_library(quark::backend::vulkan::descriptor ALIAS
            quark_backend_vulkan_descriptor)

target_link_libraries(quark_backend_vulkan_descriptor PUBLIC quark::headers::vk)

target_include_directories(quark_backend_vulkan_descriptor
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
