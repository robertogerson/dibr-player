cmake_minimum_required(VERSION 3.5)

project(dibr-player)

set(Boost_USE_STATIC_LIBS ON)
find_package( Boost 1.40 COMPONENTS program_options REQUIRED )

find_package (OpenCV REQUIRED)
find_package (OpenCL REQUIRED)

add_executable(dibr-player-cpu 
                cpu/dibr-player.cc)

target_link_libraries(dibr-player-cpu
                      ${Boost_LIBRARIES}
                      SDL
                      GL
                      vlc)


include_directories ( opencl )

add_executable( dibr-player-opencl
                opencl/yuv.cpp
                opencl/main.cpp)

target_link_libraries( dibr-player-opencl
                       general
                       ${Boost_LIBRARIES}
                       ${OpenCL_LIBRARIES}
                       ${OpenCV_LIBRARIES} )

# Copy .cl file into build dir
configure_file(opencl/convolution.cl convolution.cl COPYONLY)

