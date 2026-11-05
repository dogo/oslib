cmake_minimum_required(VERSION 3.10)
project(saveAndLoad C)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/OSLibSample.cmake")
osl_add_sample(${PROJECT_NAME} "Save And Load Test" main.c)
