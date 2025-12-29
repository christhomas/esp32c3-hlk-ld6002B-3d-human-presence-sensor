cmake_minimum_required(VERSION 3.16.0)

# Treat the existing src/ directory as the application component so app_main is built.
set(EXTRA_COMPONENT_DIRS ${CMAKE_CURRENT_LIST_DIR}/src)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hlk_ld6002)
