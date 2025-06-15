cmake_minimum_required(VERSION 3.20)

file(GLOB_RECURSE sources src/*.cpp)
add_executable(game ${sources})
target_link_libraries(game PRIVATE fly_engine)