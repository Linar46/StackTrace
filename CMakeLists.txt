cmake_minimum_required(VERSION 3.10)
project(StackTrace)

# ------------------------------------------------------------------
# Define the executable target
# ------------------------------------------------------------------
add_executable(StackTrace
    StackTrace.cpp
    StackTrace.h
)

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

# Find libdwarf headers
find_path(LIBDWARF_INCLUDE_DIR
    NAMES libdwarf.h dwarf.h
    PATHS
        /usr/include/libdwarf
)

# Find libdwarf library
find_library(LIBDWARF_LIBRARY
    NAMES dwarf libdwarf
    PATHS
        /usr/lib/x86_64-linux-gnu
        /usr/local/lib
)

# Find libelf header (yours is at /usr/include/libelf.h)
find_path(LIBELF_INCLUDE_DIR
    NAMES libelf.h
    PATHS
        /usr/include
)

# Find libelf library
find_library(LIBELF_LIBRARY
    NAMES elf libelf
    PATHS
        /usr/lib/x86_64-linux-gnu
        /usr/local/lib
)

# --- Check and link them ---
if (NOT LIBDWARF_INCLUDE_DIR OR NOT LIBDWARF_LIBRARY)
    message(FATAL_ERROR "libdwarf not found — install libdwarf-dev")
endif()

if (NOT LIBELF_INCLUDE_DIR OR NOT LIBELF_LIBRARY)
    message(FATAL_ERROR "libelf not found — install libelf-dev")
endif()

message(STATUS "Found libdwarf: ${LIBDWARF_LIBRARY} (includes at ${LIBDWARF_INCLUDE_DIR})")
message(STATUS "Found libelf: ${LIBELF_LIBRARY} (includes at ${LIBELF_INCLUDE_DIR})")

if(NOT LIBDWARF_INCLUDE_DIR)
  message(FATAL_ERROR "libdwarf headers not found. Install libdwarf-dev.")
endif()

include(${wxWidgets_USE_FILE}) # flags and libs


target_include_directories(StackTrace PUBLIC
    ${LIBDWARF_INCLUDE_DIR}
    ${LIBELF_INCLUDE_DIR}
)

target_link_libraries(StackTrace PUBLIC
    ${wxWidgets_LIBRARIES}
    ${LIBDWARF_LIBRARY}
    ${LIBELF_LIBRARY}
)


# ------------------------------------------------------------------
# Link wxWidgets libraries
# ------------------------------------------------------------------
target_link_libraries(StackTrace PUBLIC ${wxWidgets_LIBRARIES} ${LIBDWARF_LIBRARY} ${LIBELF_LIBRARY})
