cmake_minimum_required(VERSION 3.14)
project(mouse_client LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 23)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

include_directories(${CMAKE_SOURCE_DIR}/include)

# -------------------- Source Files --------------------
set(CORE_SRC
    src/main.cc
)

if(WIN32)
    file(GLOB PLATFORM_SRC src/windows/*.cc)
    set(APP_ICON_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app_icon.rc")
elseif(UNIX AND NOT APPLE)
    file(GLOB PLATFORM_SRC src/linux/*.cc)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Extra sources
set(EXTRA_SOURCES "")
if(WIN32 AND APP_ICON_RESOURCE)
    set(EXTRA_SOURCES ${APP_ICON_RESOURCE})
endif()

# -------------------- Qt Executable --------------------
qt_add_executable(${PROJECT_NAME}
    ${CORE_SRC}
    ${PLATFORM_SRC}
    resources.qrc
    ${EXTRA_SOURCES}
)

# Set executable output directory to build/bin
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Multimedia setupapi)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Multimedia)
endif()

# -------------------- Deployment Step --------------------
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${Qt6_DIR}/../../../bin/windeployqt.exe"
                --dir "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.exe"
        COMMENT "Deploying Qt DLLs and plugins into build/bin"
    )
endif()
