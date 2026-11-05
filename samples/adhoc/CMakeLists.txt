cmake_minimum_required(VERSION 3.10)
project(adhoc C)

include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/OSLibSample.cmake")
osl_add_sample(${PROJECT_NAME} "Ad hoc sample" main.c
    pspnet pspnet_apctl pspnet_adhoc pspnet_adhocctl pspnet_adhocmatching
)
