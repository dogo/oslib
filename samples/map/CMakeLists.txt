cmake_minimum_required(VERSION 3.10)
project(map C)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/OSLibSample.cmake")
osl_add_sample(${PROJECT_NAME} "Map sample" main.c)
