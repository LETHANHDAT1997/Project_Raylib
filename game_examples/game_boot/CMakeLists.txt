cmake_minimum_required(VERSION 3.16)
project(game_boot C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
# Fighter được viết bằng C++ (OOP), các game còn lại vẫn là C thuần.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
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

# raylib mặc định build KHÔNG kèm bộ giải mã JPEG (config.h: SUPPORT_FILEFORMAT_JPG 0),
# nên ảnh .jpg thả vào assets/wallpapers sẽ không nạp được. Bật lên tại đây.
# config.h dùng #ifndef nên định nghĩa từ dòng lệnh sẽ được ưu tiên.
if(TARGET raylib)
    target_compile_definitions(raylib PRIVATE SUPPORT_FILEFORMAT_JPG=1)
endif()

# Nguồn của launcher được gom theo tầng: core (logic), ui (thư viện kính),
# hub (các màn hình). Thêm file mới vào đúng thư mục là CMake tự nhận.
#
# CONFIGURE_DEPENDS bắt CMake quét lại thư mục mỗi lần build. Không có nó,
# file .c mới thêm sẽ bị bỏ qua cho tới khi chạy lại cmake bằng tay - lỗi
# link khó hiểu mà ai cũng vấp một lần.
file(GLOB BOOT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/hub/*.c"
)
file(GLOB TETRIS_SOURCES  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../tetris/src/*.c")
file(GLOB SPACE_SOURCES   CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../space_invader/src/*.c")
file(GLOB SNAKE_SOURCES   CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../snake/src/*.c")
file(GLOB FLAPPY_SOURCES  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../flappy/src/*.c")
file(GLOB CARO_SOURCES    CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../caro/src/*.c")
file(GLOB CHESS_SOURCES   CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../chess/src/*.c")
# Fighter là C++ và chia theo thư mục con (core/entities/characters/render/scenes).
file(GLOB_RECURSE FIGHTER_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../fighter/src/*.cpp")
file(GLOB COMMON_SOURCES  CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../common/*.c")

add_executable(game_boot 
    ${BOOT_SOURCES}
    ${TETRIS_SOURCES}
    ${SPACE_SOURCES}
    ${SNAKE_SOURCES}
    ${FLAPPY_SOURCES}
    ${CARO_SOURCES}
    ${CHESS_SOURCES}
    ${FIGHTER_SOURCES}
    ${COMMON_SOURCES}
)

# Định nghĩa macro IS_BUILD_ALL để các file main.c con bỏ qua hàm main()
target_compile_definitions(game_boot PRIVATE IS_BUILD_ALL)

target_include_directories(game_boot PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/hub"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common"
    "${CMAKE_CURRENT_SOURCE_DIR}/../tetris/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../space_invader/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../snake/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../flappy/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../caro/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../chess/inc"
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
