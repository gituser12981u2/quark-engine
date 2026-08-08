add_library(quark_backend_rhi_pipeline STATIC pipeline.cpp pipeline_layout.cpp)
add_library(quark::backend::rhi::pipeline ALIAS quark_backend_rhi_pipeline)

target_link_libraries(quark_backend_rhi_pipeline PUBLIC quark::headers::vk)

target_include_directories(quark_backend_rhi_pipeline
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
