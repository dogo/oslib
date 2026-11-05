cmake_minimum_required(VERSION 3.10)
project(OSK C)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/OSLibSample.cmake")
osl_add_sample(${PROJECT_NAME} "OSK test" main.c)
