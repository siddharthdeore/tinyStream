cmake_minimum_required(VERSION 3.14)

project(tinyStream LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)

find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} include)


add_executable(${CMAKE_PROJECT_NAME} main.cc)
target_link_libraries( ${CMAKE_PROJECT_NAME} ${OpenCV_LIBS} pthread)

#add_executable(raw raw.cc)
#target_link_libraries( raw ${OpenCV_LIBS}  pthread)