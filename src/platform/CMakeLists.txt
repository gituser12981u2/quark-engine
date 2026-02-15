add_subdirectory(window)

add_library(quark_platform INTERFACE)
target_link_libraries(quark_platform INTERFACE
  quark::platform::window
)

add_library(quark::platform ALIAS quark_platform)
