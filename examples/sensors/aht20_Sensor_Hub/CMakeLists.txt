# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(COMPONENT main)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(aht20_Sensor_Hub)
