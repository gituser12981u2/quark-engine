option(QUARK_HEADLESS
       "Build Quark without window/swapchain presentation support" OFF)

add_library(quark_features INTERFACE)
add_library(quark::features ALIAS quark_features)

target_compile_definitions(
  quark_features INTERFACE $<$<BOOL:${QUARK_HEADLESS}>:QUARK_HEADLESS=1>
                           $<$<NOT:$<BOOL:${QUARK_HEADLESS}>>:QUARK_HEADLESS=0>)
