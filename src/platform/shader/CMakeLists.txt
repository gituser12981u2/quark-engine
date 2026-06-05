add_library(quark_platform_shader STATIC shader_file.cpp shader_registry.cpp)
add_library(quark::platform::shader ALIAS quark_platform_shader)

target_link_libraries(quark_platform_shader PUBLIC quark::headers::core)

target_include_directories(quark_platform_shader
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
