cmake_minimum_required(VERSION 3.16)
project(caro C)

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

# Vân gỗ là ảnh .jpg (Poly Haven) - raylib mặc định tắt bộ giải mã JPEG.
if(TARGET raylib)
    target_compile_definitions(raylib PRIVATE SUPPORT_FILEFORMAT_JPG=1)
endif()

file(GLOB_RECURSE CARO_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB COMMON_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../common/*.c")

add_executable(caro ${CARO_SOURCES} ${COMMON_SOURCES})

target_include_directories(caro PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    "${CMAKE_CURRENT_SOURCE_DIR}/../common"
)

target_link_libraries(caro PRIVATE
    raylib
    m
    pthread
    dl
)

if(APPLE)
    target_link_libraries(caro PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()
