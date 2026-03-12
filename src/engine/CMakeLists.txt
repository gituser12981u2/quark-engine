add_library(quark_engine INTERFACE)
add_library(quark::engine ALIAS quark_engine)

add_subdirectory(retire)

target_link_libraries(quark_backend_vulkan
  INTERFACE
    quark::engine::retire
  PUBLIC 
    quark::headers::core
)
