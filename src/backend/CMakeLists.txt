add_library(quark_backend INTERFACE)
add_library(quark::backend ALIAS quark_backend)

add_subdirectory(vulkan)
add_subdirectory(shaders)
add_subdirectory(rhi)

target_link_libraries(quark_backend INTERFACE quark::backend::vulkan
                                              quark::backend::rhi)
