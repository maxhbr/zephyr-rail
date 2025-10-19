# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_C_FLAGS -lstdc++)
set(CMAKE_CXX_FLAGS -lstdc++)

set(BOARD xiao_nrf54l15/nrf54l15/cpuapp)

# Add DTS bindings path BEFORE find_package(Zephyr)
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../lib/stepper_with_target)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(poc_stepper_with_target)

# Add stepper_with_target library (includes the driver)
add_subdirectory(../lib/stepper_with_target ${CMAKE_CURRENT_BINARY_DIR}/stepper_with_target)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PRIVATE stepper_with_target)
