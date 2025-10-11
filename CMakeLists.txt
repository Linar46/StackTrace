cmake_minimum_required(VERSION 3.10)
project(StackTrace)

# Use modern C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

# Always build with debug symbols and disable optimizations for readable stack traces
set(CMAKE_BUILD_TYPE Debug)

# Add compile options for debug info and stack tracing
add_compile_options(-g3 -gdwarf-5 -O0 -rdynamic -no-pie -fno-omit-frame-pointer)
add_compile_definitions(wxDEBUG_LEVEL=2)

# ------------------------------------------------------------------
# Tell CMake where to find wxWidgets (non-global build)
# ------------------------------------------------------------------
set(wxWidgets_CONFIG_EXECUTABLE
    "$ENV{HOME}/src/wxWidgets-3.2.8/buildgtk/wx-config"
    CACHE FILEPATH "Path to wx-config from a local wxWidgets build"
)

# Find wxWidgets using the specified wx-config
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE}) # flags and libs

# ------------------------------------------------------------------
# Define the executable target
# ------------------------------------------------------------------
add_executable(StackTrace
    StackTrace.cpp
    StackTrace.h
)

# ------------------------------------------------------------------
# Link wxWidgets libraries
# ------------------------------------------------------------------
target_link_libraries(StackTrace PUBLIC ${wxWidgets_LIBRARIES})
