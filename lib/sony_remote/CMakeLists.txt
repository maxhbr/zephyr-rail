# Sony Remote Library for Zephyr

# Create the library target
add_library(sony_remote STATIC)

target_sources(sony_remote PRIVATE
    src/sony_remote.cpp
)

target_include_directories(sony_remote PUBLIC
    include
)

# Compile options for C++17
target_compile_options(sony_remote PRIVATE -lstdc++ -std=c++17 -fpermissive)

# Link with Zephyr kernel for access to Zephyr APIs
target_link_libraries(sony_remote PUBLIC zephyr_interface)