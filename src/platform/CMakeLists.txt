add_library(quark_platform INTERFACE)

if(NOT QUARK_HEADLESS)
  add_subdirectory(window)
  target_link_libraries(quark_platform INTERFACE quark::platform::window)
endif()

add_library(quark::platform ALIAS quark_platform)
