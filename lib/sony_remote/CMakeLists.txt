# Sony Remote Library for Zephyr

# Create the library target
add_library(sony_remote STATIC)

# Conditionally compile based on CONFIG_BT
if(CONFIG_BT)
    target_sources(sony_remote PRIVATE
        src/sony_remote.cpp
    )
else()
    target_sources(sony_remote PRIVATE
        src/fake_sony_remote.cpp
    )
endif()

target_include_directories(sony_remote PUBLIC
    include
)

# Compile options for C++17
target_compile_options(sony_remote PRIVATE -lstdc++ -std=c++17 -fpermissive)

# Link with Zephyr kernel for access to Zephyr APIs
target_link_libraries(sony_remote PUBLIC zephyr_interface)