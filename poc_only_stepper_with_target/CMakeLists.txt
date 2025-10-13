cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_C_FLAGS -lstdc++)
set(CMAKE_CXX_FLAGS -lstdc++)

set(BOARD xiao_ble/nrf52840/sense)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(poc_stepper_with_target)

zephyr_library_compile_options(-lstdc++ -std=c++17 -fpermissive)

add_subdirectory(../lib/stepper_with_target ${CMAKE_CURRENT_BINARY_DIR}/stepper_with_target)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PRIVATE stepper_with_target)
