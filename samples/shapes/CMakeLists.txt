cmake_minimum_required(VERSION 3.10)
project(shapes C CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/OSLibSample.cmake")
osl_add_sample(${PROJECT_NAME} "Shapes Sample" main.cpp)
