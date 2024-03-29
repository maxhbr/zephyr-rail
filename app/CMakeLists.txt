# SPDX-License-Identifier: Apache-2.0

# Re-direct the directory where the 'boards' directory is found from
# $ZEPHYR_BASE to this directory.
# set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})


# set(BOARD wio_terminal)
# set(BOARD nucleo_f767zi)
# set(SHIELD adafruit_2_8_tft_touch_v2)

set(CMAKE_C_FLAGS -lstdc++)
set(CMAKE_CXX_FLAGS -lstdc++)

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(zephyr-rail)

# zephyr_library_compile_options(-lstdc++ -std=c++17 -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-variable -Wno-comment)
zephyr_library_compile_options(-lstdc++ -std=c++17 -fpermissive)


FILE(GLOB app_sources src/*.cpp src/stepper/*.cpp src/display/*.cpp src/state/*.cpp)
target_sources(app PRIVATE ${app_sources})
