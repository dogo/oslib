cmake_minimum_required(VERSION 3.10)
project(net C)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/OSLibSample.cmake")
osl_add_sample(${PROJECT_NAME} "Net test" main.c
    pspnet pspnet_apctl pspnet_inet pspnet_resolver
)
