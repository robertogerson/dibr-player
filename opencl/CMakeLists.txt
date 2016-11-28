cmake_minimum_required(VERSION 3.5)

project(dibr-player)

include_directories(.)

find_package( OpenCV REQUIRED )
find_package( OpenCL REQUIRED )

add_executable(dibr-player 
                yuv.cpp
                main.cpp)

target_link_libraries(dibr-player
                      general
                      ${OpenCL_LIBRARIES}
                      ${OpenCV_LIBRARIES})

