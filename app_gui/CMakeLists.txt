# SPDX-License-Identifier: Apache-2.0

# set(BOARD stm32h7b3i_dk)
# set(SHIELD x_nucleo_idb05a1)

set(CMAKE_C_FLAGS -lstdc++)
set(CMAKE_CXX_FLAGS -lstdc++)

cmake_minimum_required(VERSION 3.20.0)

# Add DTS bindings from libraries
list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../lib/stepper_with_target)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr-rail)

# zephyr_library_compile_options(-lstdc++ -std=c++17 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-variable -Wno-comment)
zephyr_library_compile_options(-lstdc++ -std=c++17 -fpermissive)

# Add the sony_remote library
add_subdirectory(../lib/sony_remote ${CMAKE_CURRENT_BINARY_DIR}/sony_remote)

# Add the stepper_with_target library
add_subdirectory(../lib/stepper_with_target ${CMAKE_CURRENT_BINARY_DIR}/stepper_with_target)

FILE(GLOB app_sources src/*.cpp)
# Remove sony_remote files from glob
list(FILTER app_sources EXCLUDE REGEX ".*sony_remote\\.cpp$")
# Remove stepper related files from glob
list(FILTER app_sources EXCLUDE REGEX ".*(Stepper|GPIOs)\\.cpp$")
target_sources(app PRIVATE ${app_sources})

# Link the libraries
target_link_libraries(app PRIVATE sony_remote stepper_with_target)
