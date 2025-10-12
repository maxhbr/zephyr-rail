# SPDX-License-Identifier: Apache-2.0

# set(BOARD stm32h7b3i_dk)
# set(SHIELD x_nucleo_idb05a1)

set(CMAKE_C_FLAGS -lstdc++)
set(CMAKE_CXX_FLAGS -lstdc++)

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr-rail)

# zephyr_library_compile_options(-lstdc++ -std=c++17 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-variable -Wno-comment)
zephyr_library_compile_options(-lstdc++ -std=c++17 -fpermissive)

# Add the sony_remote library
add_subdirectory(../lib/sony_remote ${CMAKE_CURRENT_BINARY_DIR}/sony_remote)

FILE(GLOB app_sources src/*.cpp)
# Remove sony_remote files from glob
list(FILTER app_sources EXCLUDE REGEX ".*sony_remote\\.cpp$")
target_sources(app PRIVATE ${app_sources})

# Link the sony_remote library
target_link_libraries(app PRIVATE sony_remote)
