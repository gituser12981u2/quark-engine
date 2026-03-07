add_library(quark_engine_retire STATIC
  retirement_queue.cpp

)
add_library(quark::engine::retire ALIAS quark_engine_retire)

target_link_libraries(quark_engine_retire
  PUBLIC
    quark::headers::core
)

target_include_directories(quark_engine_retire
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
