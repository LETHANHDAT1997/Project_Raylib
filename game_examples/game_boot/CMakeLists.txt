cmake_minimum_required(VERSION 3.16)
project(game_boot C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find or include raylib
if(NOT TARGET raylib)
    get_filename_component(RAYLIB_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../raylib" ABSOLUTE)
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

file(GLOB BOOT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB TETRIS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../tetris/src/*.c")
file(GLOB SPACE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../space_invader/src/*.c")
file(GLOB SNAKE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../snake/src/*.c")
file(GLOB FIGHTER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../fighter/src/*.c")
file(GLOB COMMON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../common/*.c")

add_executable(game_boot 
    ${BOOT_SOURCES}
    ${TETRIS_SOURCES}
    ${SPACE_SOURCES}
    ${SNAKE_SOURCES}
    ${FIGHTER_SOURCES}
    ${COMMON_SOURCES}
)

# Định nghĩa macro IS_BUILD_ALL để các file main.c con bỏ qua hàm main()
target_compile_definitions(game_boot PRIVATE IS_BUILD_ALL)

target_include_directories(game_boot PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tetris/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../space_invader/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../snake/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../fighter/inc"
)

target_link_libraries(game_boot PRIVATE
    raylib
    m
    pthread
    dl
)

if(APPLE)
    target_link_libraries(game_boot PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()
