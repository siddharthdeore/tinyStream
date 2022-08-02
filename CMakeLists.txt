cmake_minimum_required(VERSION 3.14)

project(stream LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} include)


add_executable(main main.cc)
target_link_libraries( main ${OpenCV_LIBS} pthread)

#add_executable(single_old single_old.cc)
#target_link_libraries( single_old ${OpenCV_LIBS}  pthread)