add_library(quark_platform INTERFACE)
add_library(quark::platform ALIAS quark_platform)

add_subdirectory(shader)

if(NOT QUARK_HEADLESS)
  add_subdirectory(window)
  target_link_libraries(quark_platform INTERFACE quark::platform::window)
endif()

target_link_libraries(quark_platform INTERFACE quark::platform::shader)
