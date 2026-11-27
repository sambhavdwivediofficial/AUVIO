cmake_minimum_required(VERSION 3.21)

option(AUVIO_BUILD_TESTS "Build unit tests" ON)
if (AUVIO_BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(AUVIO
    VERSION 0.1.0
    DESCRIPTION "Lightweight cross-platform video and music player"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 COMPONENTS Widgets REQUIRED)
set(AUVIO_QT_LIBS Qt6::Widgets)

find_package(FFMPEG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

if (WIN32)
    set(AUVIO_PLATFORM_SOURCES src/platform/windows/HardwareDetect_Win.cpp)
    set(AUVIO_PLATFORM_INCLUDE_DIR src/platform/windows)
else()
    set(AUVIO_PLATFORM_SOURCES src/platform/linux/HardwareDetect_Linux.cpp)
    set(AUVIO_PLATFORM_INCLUDE_DIR src/platform/linux)
endif()

set(AUVIO_SOURCES
    src/main.cpp
    src/core/Demuxer.cpp
    src/core/Decoder.cpp
    src/core/AudioEngine.cpp
    src/core/VideoRenderer.cpp
    src/core/PlaybackController.cpp
    src/ui/MainWindow.cpp
    src/ui/PlayerControls.cpp
    src/ui/PlaylistView.cpp
    src/ui/SettingsDialog.cpp
    src/utils/Logger.cpp
    src/utils/Config.cpp
    ${AUVIO_PLATFORM_SOURCES}
)

if (WIN32)
    list(APPEND AUVIO_SOURCES packaging/windows/auvio.rc)
endif()

add_executable(AUVIO WIN32 ${AUVIO_SOURCES} resources.qrc)

target_include_directories(AUVIO PRIVATE
    src/core
    src/ui
    src/utils
    ${AUVIO_PLATFORM_INCLUDE_DIR}
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_directories(AUVIO PRIVATE ${FFMPEG_LIBRARY_DIRS})

target_link_libraries(AUVIO PRIVATE
    ${AUVIO_QT_LIBS}
    ${FFMPEG_LIBRARIES}
    SDL2::SDL2
    SDL2::SDL2main
)

if (WIN32)
    target_link_libraries(AUVIO PRIVATE dxgi)
endif()

set_target_properties(AUVIO PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if (AUVIO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
