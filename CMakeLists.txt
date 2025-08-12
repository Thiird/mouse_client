cmake_minimum_required(VERSION 3.14)
project(mouse_client LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)

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

# Link Qt libraries and SetupAPI for Windows
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Multimedia setupapi)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Multimedia)
endif()