# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

# set(COMPONENT_ADD_INCLUDEDIRS "$ENV{IDF_PATH}/examples")
                           
# set(EXTRA_COMPONENT_DIRS "$ENV{COMMON}")
# include_directories($ENV{COMMON}/**) 
# # 

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(REMOTE)