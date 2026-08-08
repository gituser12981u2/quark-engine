add_library(quark_backend_rhi INTERFACE)
add_library(quark::backend::rhi ALIAS quark_backend_rhi)

add_subdirectory(descriptor)
add_subdirectory(pipeline)
add_subdirectory(shader)

target_link_libraries(
  quark_backend_rhi INTERFACE quark::backend::rhi::descriptor
                              quark::backend::rhi::pipeline quark::rhi::shader)
