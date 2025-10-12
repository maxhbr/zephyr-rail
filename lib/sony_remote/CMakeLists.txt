# Sony Remote Library for Zephyr

zephyr_library()

zephyr_library_sources(
    src/sony_remote.cpp
)

zephyr_library_include_directories(include)

# Compile options for C++17
zephyr_library_compile_options(-lstdc++ -std=c++17 -fpermissive)

# Link libraries for Bluetooth support
zephyr_library_link_libraries(INTERFACE zephyr_interface)