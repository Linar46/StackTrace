cmake_minimum_required(VERSION 3.10)

project(StackTrace)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Always build with debug symbols and no optimization for readable stack traces
set(CMAKE_BUILD_TYPE Debug)

# Add compile options for debug info
add_compile_options(-g -O0 -rdynamic)
add_compile_definitions(wxDEBUG_LEVEL=2)
# Find wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})

# Define the executable
add_executable(StackTrace StackTrace.cpp StackTrace.h)

# Link with wxWidgets
target_link_libraries(StackTrace PUBLIC ${wxWidgets_LIBRARIES})
