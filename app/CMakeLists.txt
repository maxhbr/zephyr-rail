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


FILE(GLOB app_sources src/*.cpp)
target_sources(app PRIVATE ${app_sources})
