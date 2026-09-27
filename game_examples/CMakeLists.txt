cmake_minimum_required(VERSION 3.16)
project(game_examples C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
# fighter dùng C++17; các example khác vẫn là C thuần.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

# raylib mặc định build KHÔNG kèm bộ giải mã JPEG (config.h: SUPPORT_FILEFORMAT_JPG 0),
# nên ảnh .jpg thả vào assets/wallpapers sẽ không nạp được. Bật lên tại đây.
# config.h dùng #ifndef nên định nghĩa từ dòng lệnh sẽ được ưu tiên.
if(TARGET raylib)
    target_compile_definitions(raylib PRIVATE SUPPORT_FILEFORMAT_JPG=1)
endif()

# Add each game subdirectory
add_subdirectory(tetris)
add_subdirectory(space_invader)
add_subdirectory(snake)
add_subdirectory(game_boot)
add_subdirectory(fighter)
