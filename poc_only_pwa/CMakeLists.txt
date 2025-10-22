cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_C_FLAGS -lstdc++)
set(CMAKE_CXX_FLAGS -lstdc++)

set(BOARD xiao_nrf54l15/nrf54l15/cpuapp)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(poc_pwa)

zephyr_library_compile_options(-lstdc++ -std=c++17 -fpermissive)

target_sources(app PRIVATE 
  src/main.cpp
  src/pwa_service.cpp
)
