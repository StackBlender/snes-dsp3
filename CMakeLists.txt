# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) StackBlender LLC
cmake_minimum_required(VERSION 3.16)
project(snes_dsp3 CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dsp3 STATIC src/dsp3/Dsp3.cpp src/dsp3/Decoder.cpp)
target_include_directories(dsp3 PUBLIC src)

add_executable(dsp3_check tools/dsp3_check.cpp)
target_link_libraries(dsp3_check PRIVATE dsp3)

enable_testing()
add_executable(dsp3_state_test tests/state_test.cpp)
target_link_libraries(dsp3_state_test PRIVATE dsp3)
add_test(NAME dsp3_state COMMAND dsp3_state_test)
