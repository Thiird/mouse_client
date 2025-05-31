cmake_minimum_required(VERSION 4.0)
project(MyCppProject LANGUAGES CXX)

set(PROJ_VAR mouse_client)

# Set C++23 standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Define the executable
add_executable(${PROJ_VAR} src/main.cc)

# Include directories
target_include_directories(${PROJ_VAR} PRIVATE ${CMAKE_SOURCE_DIR}/src/include)

# Platform-specific source files
if(WIN32)
    message(STATUS "Configuring for Windows")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Configuring for Linux")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()