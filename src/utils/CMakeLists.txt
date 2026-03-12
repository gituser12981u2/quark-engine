add_library(quark_utils INTERFACE)
add_library(quark::utils ALIAS quark_utils)

add_subdirectory(diagnostics)

target_link_libraries(quark_utils
  INTERFACE
    quark::utils::diagnostics
    fmt::fmt
    tl::expected
)
