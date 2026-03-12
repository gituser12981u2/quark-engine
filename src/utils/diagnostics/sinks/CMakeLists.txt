add_library(quark_utils_sinks STATIC 
  default_sink.cpp
  spdlog_sink.cpp
)
add_library(quark::utils::sinks ALIAS quark_utils_sinks)

target_link_libraries(quark_utils_sinks
  PUBLIC 
    quark::headers::core
)

target_include_directories(quark_utils_sinks
  PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
