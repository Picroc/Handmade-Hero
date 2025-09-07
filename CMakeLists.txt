cmake_minimum_required(VERSION 4.0)
project(Handmade_Hero)

set(CMAKE_CXX_STANDARD 20)

include_directories(/opt/homebrew/include)

add_executable(Handmade_Hero main.cpp)
target_link_libraries(Handmade_Hero /opt/homebrew/lib/libSDL2-2.0.0.dylib)
