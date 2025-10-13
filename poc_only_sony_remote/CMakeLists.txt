cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_C_FLAGS -lstdc++)
set(CMAKE_CXX_FLAGS -lstdc++)

# set(BOARD b_l4s5i_iot01a)
set(BOARD xiao_ble/nrf52840/sense)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(poc_sony_remote)

# Compile options for C++17
zephyr_library_compile_options(-lstdc++ -std=c++17 -fpermissive)

# Add the sony_remote library
add_subdirectory(../lib/sony_remote ${CMAKE_CURRENT_BINARY_DIR}/sony_remote)

target_sources(app PRIVATE src/main.cpp)

# Link the sony_remote library
target_link_libraries(app PRIVATE sony_remote)
