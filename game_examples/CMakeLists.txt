cmake_minimum_required(VERSION 3.16)
project(game_examples C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Configure raylib once for all game examples
if(NOT TARGET raylib)
    get_filename_component(RAYLIB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../raylib" ABSOLUTE)
    if(EXISTS "${RAYLIB_ROOT_DIR}/CMakeLists.txt")
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        add_subdirectory("${RAYLIB_ROOT_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/raylib_build")
    else()
        find_package(raylib QUIET)
        if(NOT raylib_FOUND)
            include(FetchContent)
            FetchContent_Declare(
                raylib
                GIT_REPOSITORY https://github.com/raysan5/raylib.git
                GIT_TAG master
            )
            FetchContent_MakeAvailable(raylib)
        endif()
    endif()
endif()

# Add each game subdirectory
add_subdirectory(tetris)
add_subdirectory(space_invader)
