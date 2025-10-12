# Stepper with Target Library for Zephyr

# Create the library target
add_library(stepper_with_target STATIC)

target_sources(stepper_with_target PRIVATE
    src/GPIOs.cpp
    src/Stepper.cpp
    src/StepperWithTarget.cpp
)

target_include_directories(stepper_with_target PUBLIC
    include
)

# Compile options for C++17
target_compile_options(stepper_with_target PRIVATE -lstdc++ -std=c++17 -fpermissive)

# Link with Zephyr kernel for access to Zephyr APIs
target_link_libraries(stepper_with_target PUBLIC zephyr_interface)

# Add DTS bindings to the global DTS path for Zephyr
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(DTS_ROOT ${DTS_ROOT} PARENT_SCOPE)