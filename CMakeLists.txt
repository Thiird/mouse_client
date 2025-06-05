cmake_minimum_required(VERSION 4.0)
project(mouse_client CXX)

# Set include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Core source files (OS-agnostic)
set(CORE_SRC
    src/main.cc
)

# OS-specific source files
if(WIN32)
    set(PLATFORM_SRC src/windows/com_port.cc)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_SRC src/linux/com_port.cc)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Combine sources
set(ALL_SRC ${CORE_SRC} ${PLATFORM_SRC})

# Create executable
add_executable(${PROJECT_NAME} ${ALL_SRC})