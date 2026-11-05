cmake_minimum_required(VERSION 3.10)
project(helloWorld C)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/OSLibSample.cmake")
osl_add_sample(${PROJECT_NAME} "Hello World" main.c
    pspnet_adhoc pspnet_adhocctl pspnet_adhocmatching
)
