cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_C_FLAGS -lstdc++)
set(CMAKE_CXX_FLAGS -lstdc++)

set(BOARD xiao_nrf54l15/nrf54l15/cpuapp)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(poc_sony_remote)

zephyr_library_compile_options(-lstdc++ -std=c++17 -fpermissive)

add_subdirectory(../lib/sony_remote ${CMAKE_CURRENT_BINARY_DIR}/sony_remote)

target_sources(app PRIVATE src/main.cpp)

target_link_libraries(app PRIVATE sony_remote)
