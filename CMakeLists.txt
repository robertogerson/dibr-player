cmake_minimum_required(VERSION 3.5)

project(dibr-player)

add_executable(dibr-player-cpu 
                cpu/dibr-player.cc)

target_link_libraries(dibr-player-cpu
                      general
                      SDL
                      GL
                      vlc)


find_package (OpenCV REQUIRED)
find_package (OpenCL REQUIRED)

include_directories ( opencl )

add_executable(dibr-player-opencl
                opencl/yuv.cpp
                opencl/main.cpp)

target_link_libraries( dibr-player-opencl
                       general
                       ${OpenCL_LIBRARIES}
                       ${OpenCV_LIBRARIES} )


