cmake_minimum_required(VERSION 3.10)

project(StackTrace)

set(CMAKE_CXX_STANDARD 17)

find_package(wxWidgets REQUIRED COMPONENTS core base)

include(${wxWidgets_USE_FILE})

add_executable(StackTrace StackTrace.cpp StackTrace.h)

target_link_libraries(StackTrace ${wxWidgets_LIBRARIES})
