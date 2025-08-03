cmake_minimum_required(VERSION 3.14)
project(mouse_client LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add Multimedia to required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

include_directories(${CMAKE_SOURCE_DIR}/include)

set(CORE_SRC
    src/main.cc
)

# Platform-specific sources
if(WIN32)
    file(GLOB PLATFORM_SRC src/windows/*.cc)
elseif(UNIX AND NOT APPLE)
    file(GLOB PLATFORM_SRC src/linux/*.cc)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Add Qt resources
qt_add_executable(${PROJECT_NAME}
    ${CORE_SRC}
    ${PLATFORM_SRC}
    resources.qrc
)

# Link Qt libraries (add Multimedia)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Multimedia)
