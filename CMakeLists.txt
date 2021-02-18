cmake_minimum_required(VERSION 3.0.0)

if (MSVC)
    set(CMAKE_TOOLCHAIN_FILE "D:/Programming/Utilities/vcpkg/scripts/buildsystems/vcpkg.cmake")
elseif (UNIX)
    set(CMAKE_TOOLCHAIN_FILE "~/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

project(ImageDuplicateDetector-C VERSION 0.1.0)

if (MSVC)
    add_compile_definitions(WINDOWS)
    add_compile_options(/std:c++17)
elseif (UNIX)
    add_compile_definitions(UNIX)
    add_compile_options(-std=c++17)
    set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/unix)
endif()

include(CTest)
enable_testing()

find_package(OpenCV CONFIG REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories("./include")

add_executable(ImageDuplicateDetector-C main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
target_link_libraries(ImageDuplicateDetector-C ${OpenCV_LIBS})
