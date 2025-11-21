cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(state_machine_tests)

set(APP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(REPO_ROOT ${APP_ROOT}/..)

set(TEST_SOURCES
    ${APP_ROOT}/src/StateMachine.cpp
    ${APP_ROOT}/src/Stack.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/fake_stepper_with_target.cpp
    ${REPO_ROOT}/lib/sony_remote/src/fake_sony_remote.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

target_sources(app PRIVATE ${TEST_SOURCES})

target_include_directories(app PRIVATE
    ${APP_ROOT}/src
    ${REPO_ROOT}/lib/stepper_with_target/include
    ${REPO_ROOT}/lib/sony_remote/include
)

target_compile_options(app PRIVATE -std=c++17 -fpermissive)
