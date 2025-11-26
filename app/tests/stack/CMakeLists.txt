cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(stack_tests)

set(APP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(REPO_ROOT ${APP_ROOT}/..)

# Reuse production sources for Stack behavior
set(STACK_SOURCES
    ${APP_ROOT}/src/Stack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_sources(app PRIVATE ${STACK_SOURCES})

target_include_directories(app PRIVATE
    ${APP_ROOT}/src
    ${REPO_ROOT}/lib/stepper_with_target/include
)

target_compile_options(app PRIVATE -std=c++17 -fpermissive)
