add_library(quark_backend INTERFACE)
add_library(quark::backend ALIAS quark_backend)

add_subdirectory(vulkan)
add_subdirectory(shaders/basic_triangle)

target_link_libraries(quark_backend
  INTERFACE
    quark::backend::vulkan
)
