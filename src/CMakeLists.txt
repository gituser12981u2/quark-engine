add_executable(quark main.cpp)

set_target_properties(quark PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_compile_options(quark PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Wshadow -pedantic-errors>
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_custom_target(run 
  COMMAND $<TARGET_FILE:quark>
  DEPENDS quark 
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

quark_apply_sanitizers(quark)
quark_apply_lto(quark)
