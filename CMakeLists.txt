cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME handmade-hero)
project(${PROJECT_NAME})

# compiler setup
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DHANDMADE_WIN32=1 -FC")

# deps
#find_package(MyLibraryDependencyName REQUIRED)

# sources
include_directories(include) # папка include, в ней my-project-name/*.h хедеры
include_directories(include/platform include/game)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp) # папка src, в ней *.cpp

# binary
add_executable(${PROJECT_NAME} ${SOURCES})

# link dependencies
#target_link_libraries(${PROJECT_NAME} PRIVATE
#        MyLibraryDependencyName::MyLibraryDependencyName
#)
