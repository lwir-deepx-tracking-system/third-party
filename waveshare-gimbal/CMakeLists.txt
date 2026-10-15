
cmake_minimum_required(VERSION 3.16)
project(waveshare_gimbal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(gimbal_test main.cpp)

target_compile_options(gimbal_test PRIVATE
    -Wall -Wextra -Wpedantic
)
