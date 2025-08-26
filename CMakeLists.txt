cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME handmade)
project(${PROJECT_NAME})

# compiler setup
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(HANDMADE_INTERNAL=1 HANDMADE_SLOW=1 WIN32_HANDMADE=1)
add_compile_options(
        -Wall -Wconversion -Wno-write-strings -Wno-unused-function -Wno-unused-variable -Wno-sign-compare -Wno-format
        -march=native
        -fno-rtti -fno-exceptions
        -mwindows
)
# deps
#find_package(MyLibraryDependencyName REQUIRED)

# sources
include_directories(include) # папка include, в ней my-project-name/*.h хедеры
include_directories(include/platform include/game)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/win32_handmade.cpp) # папка src, в ней *.cpp
file(GLOB_RECURSE DLL_SOURCES CONFIGURE_DEPENDS src/handmade.cpp)

# binary
add_library(handmade SHARED ${DLL_SOURCES})
add_executable(win32_handmade ${SOURCES})

# link dependencies
target_link_libraries(win32_handmade PRIVATE
        winmm
        handmade
)
