add_library(quark_backend_rhi_descriptor STATIC descriptor_set_layout.cpp)
add_library(quark::backend::rhi::descriptor ALIAS quark_backend_rhi_descriptor)

target_link_libraries(quark_backend_rhi_descriptor PUBLIC quark::headers::vk)

target_include_directories(quark_backend_rhi_descriptor
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
