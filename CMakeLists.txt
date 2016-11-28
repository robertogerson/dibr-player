cmake_minimum_required(VERSION 3.5)

project(dibr-player)

add_executable(dibr-player 
                dibr-player.cc)

target_link_libraries(dibr-player general SDL GL vlc)

