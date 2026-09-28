idf_component_register(
    SRCS "main.cpp" "scene.cpp"
    INCLUDE_DIRS "."
    REQUIRES shapogfx m5unified m5gfx esp_timer
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra)

# idf.py -DMEOW_FAST=0 build で低速版。省略時は config.hpp の既定 (高速版)。
if(DEFINED MEOW_FAST)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE MEOW_FAST=${MEOW_FAST})
endif()
