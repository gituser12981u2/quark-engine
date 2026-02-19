add_library(quark_utils STATIC 
  diagnostic.cpp
)
add_library(quark::utils ALIAS quark_utils)

add_subdirectory(sinks)

target_link_libraries(quark_utils
  INTERFACE
    quark::utils::sinks
  PUBLIC 
    quark::headers
    fmt::fmt
)

target_include_directories(quark_utils
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
