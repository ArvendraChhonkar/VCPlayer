cmake_minimum_required(VERSION 3.16)
project(MyPlayer VERSION 0.1 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PkgConfig
find_package(PkgConfig REQUIRED)

# Use pkg-config to find SDL2
pkg_check_modules(SDL2 REQUIRED sdl2)

# Use pkg-config to find FFmpeg libraries
pkg_check_modules(FFMPEG REQUIRED
    libavformat
    libavcodec
    libavutil
    libswscale
    libswresample
)

# Include directories
include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    imgui
    imgui/backends
    ImGuiFileDialog
)

# Link directories (optional for static or non-standard installs)
link_directories(
    ${SDL2_LIBRARY_DIRS}
    ${FFMPEG_LIBRARY_DIRS}
)

# Source files
set(SOURCES
    main.cpp
    App.cpp
    App.h
    VideoPlayer.cpp
    VideoPlayer.h
    FileDialog.cpp
    FileDialog.h

    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_demo.cpp

    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_sdlrenderer2.cpp

    ImGuiFileDialog/ImGuiFileDialog.cpp
    ImGuiFileDialog/ImGuiFileDialog.h
)

# Create executable
add_executable(MyPlayer ${SOURCES})

# Link libraries
target_link_libraries(MyPlayer
    ${SDL2_LIBRARIES}
    ${FFMPEG_LIBRARIES}

    # Explicit FFmpeg libraries (solve undefined references)
    avformat
    avcodec
    avutil
    swscale
    swresample

    # System dependencies
    pthread
    dl
)
