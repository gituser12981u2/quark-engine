add_library(quark_utils_diagnostics STATIC 
  diagnostic.cpp
)
add_library(quark::utils::diagnostics ALIAS quark_utils_diagnostics)

add_subdirectory(sinks)

target_link_libraries(quark_utils_diagnostics
  PUBLIC 
    tl::expected
  PRIVATE
    quark::utils::sinks
)

target_include_directories(quark_utils_diagnostics
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
