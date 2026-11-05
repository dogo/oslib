cmake_minimum_required(VERSION 3.10)
project(rectAndFonts C)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/OSLibSample.cmake")
osl_add_sample(${PROJECT_NAME} "Rect and Fonts Test" main.c)
