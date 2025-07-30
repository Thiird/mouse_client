cmake_minimum_required(VERSION 3.14)
project(mouse_client LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)  # <-- THIS enables automatic moc generation for Q_OBJECT anywhere, even in .cc

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Always add the core source
set(CORE_SRC
    src/main.cc
)

# OS-specific: add all *.cc files from the relevant folder
if(WIN32)
    file(GLOB PLATFORM_SRC src/windows/*.cc)
elseif(UNIX AND NOT APPLE)
    file(GLOB PLATFORM_SRC src/linux/*.cc)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Combine sources
set(ALL_SRC ${CORE_SRC} ${PLATFORM_SRC})

# Create executable
add_executable(${PROJECT_NAME} ${ALL_SRC})

# Link Qt
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)
