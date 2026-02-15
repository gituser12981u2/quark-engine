add_library(quark_platform_window STATIC 
  glfw_window.cpp
)
add_library(quark::platform::window ALIAS quark_platform_window)

target_link_libraries(quark_platform_window
  PUBLIC
    quark::headers
  PRIVATE 
    Vulkan::Vulkan
    glfw
)

target_include_directories(quark_platform_window
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
