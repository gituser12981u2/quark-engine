add_library(quark_build_settings INTERFACE)
add_library(quark::build_settings ALIAS quark_build_settings)

target_include_directories(quark_build_settings INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
target_compile_features(quark_build_settings INTERFACE cxx_std_23)
target_compile_options(quark_build_settings INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /Zc:preprocessor>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -pedantic-errors>
)

add_library(quark_headers_core INTERFACE)
add_library(quark::headers::core ALIAS quark_headers_core)

target_link_libraries(quark_headers_core
  INTERFACE
    quark::build_settings
    fmt::fmt
)

add_library(quark_headers_vk INTERFACE)
add_library(quark::headers::vk ALIAS quark_headers_vk)
target_link_libraries(quark_headers_vk
  INTERFACE
    quark::headers::core
    Vulkan::Vulkan
)

add_subdirectory(backend)
add_subdirectory(engine)
add_subdirectory(platform)
add_subdirectory(utils)

add_executable(quark_app main.cpp)
set_target_properties(quark_app PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(quark_app PRIVATE 
    quark::backend
    quark::engine
    quark::platform
    quark::utils
)

target_compile_options(quark_app PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive /Zc:preprocessor->
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -pedantic-errors>
)

add_custom_target(run
  COMMAND $<TARGET_FILE:quark_app>
  DEPENDS quark_app
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  USES_TERMINAL
)

quark_apply_sanitizers(quark_app)
quark_apply_lto(quark_app)
