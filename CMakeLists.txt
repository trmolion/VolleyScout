cmake_minimum_required(VERSION 3.21)

project(VolleyScout VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(VOLLEYSCOUT_BUILD_TESTS "Build unit tests for the core library" ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui GuiPrivate Widgets Svg Multimedia MultimediaWidgets)
qt_standard_project_setup()

include(FetchContent)
FetchContent_Declare(
    QXlsx
    GIT_REPOSITORY https://github.com/QtExcel/QXlsx.git
    GIT_TAG        v1.5.1.1
    GIT_SHALLOW    TRUE
    SOURCE_SUBDIR  QXlsx
)
set(QT_VERSION_MAJOR 6 CACHE STRING "" FORCE)
FetchContent_MakeAvailable(QXlsx)

if(VOLLEYSCOUT_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(src)
