add_library(quark_headers INTERFACE)
add_library(quark::headers ALIAS quark_headers)

target_include_directories(quark_headers INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(quark_headers INTERFACE cxx_std_23)

add_subdirectory(backend)
add_subdirectory(platform)

add_executable(quark_app main.cpp)
set_target_properties(quark_app PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(quark_app PRIVATE 
    quark::backend
    quark::platform
)

target_compile_options(quark_app PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -pedantic-errors>
)

add_custom_target(run
  COMMAND $<TARGET_FILE:quark_app>
  DEPENDS quark_app
  WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

quark_apply_sanitizers(quark_app)
quark_apply_lto(quark_app)
