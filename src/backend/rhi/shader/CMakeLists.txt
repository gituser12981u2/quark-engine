add_library(quark_rhi_shader STATIC shader_file.cpp shader_registry.cpp)
add_library(quark::rhi::shader ALIAS quark_rhi_shader)

target_link_libraries(quark_rhi_shader PUBLIC quark::headers::core)

target_include_directories(quark_rhi_shader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
