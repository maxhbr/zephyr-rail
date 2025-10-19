# Stepper with Target Library for Zephyr

# Create the library target
add_library(stepper_with_target STATIC)

target_sources(stepper_with_target PRIVATE
    src/StepperWithTarget.cpp
)

# Add driver sources if SIMPLE_STEPPER is enabled
if(CONFIG_SIMPLE_STEPPER)
    target_sources(stepper_with_target PRIVATE
        drivers/stepper/simple_stepper.c
        ${ZEPHYR_BASE}/drivers/stepper/step_dir/step_dir_stepper_common.c
        ${ZEPHYR_BASE}/drivers/stepper/step_dir/step_dir_stepper_work_timing.c
    )
endif()

target_include_directories(stepper_with_target PUBLIC
    include
)

# Compile options for C++17 - only for C++ sources
target_compile_options(stepper_with_target PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-lstdc++>
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++17>
    $<$<COMPILE_LANGUAGE:CXX>:-fpermissive>
)

# Link with Zephyr kernel for access to Zephyr APIs
target_link_libraries(stepper_with_target PUBLIC zephyr_interface)

# Add DTS bindings to the global DTS path for Zephyr
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(DTS_ROOT ${DTS_ROOT} PARENT_SCOPE)