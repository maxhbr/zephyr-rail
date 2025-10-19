# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_C_FLAGS -lstdc++)
set(CMAKE_CXX_FLAGS -lstdc++)

set(BOARD xiao_nrf54l15/nrf54l15/cpuapp)

# Add DTS bindings path BEFORE find_package(Zephyr)
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../lib/stepper_with_target)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr-rail)

zephyr_library_compile_options(-lstdc++ -std=c++17 -fpermissive)

# Add the sony_remote library
add_subdirectory(../lib/sony_remote ${CMAKE_CURRENT_BINARY_DIR}/sony_remote)

# Add stepper_with_target library (includes the driver)
add_subdirectory(../lib/stepper_with_target ${CMAKE_CURRENT_BINARY_DIR}/stepper_with_target)

FILE(GLOB app_sources src/*.cpp)
target_sources(app PRIVATE ${app_sources})

# Link the libraries
target_link_libraries(app PRIVATE sony_remote stepper_with_target)
