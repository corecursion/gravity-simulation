# CMakeLists.txt
# See also: https://developer.nvidia.com/blog/building-cuda-applications-cmake/

cmake_minimum_required(VERSION 3.25.2 FATAL_ERROR)    # 3.25.2 required for nvcc C++20

if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
  set(CMAKE_CUDA_ARCHITECTURES 75)
endif()

project(gravity-simulation LANGUAGES C CXX CUDA)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

add_executable(gravity-simulation deps/glad/src/glad.c gravity-simulation.cu)

target_compile_features(gravity-simulation PUBLIC cxx_std_20)

target_include_directories(gravity-simulation PUBLIC deps/csv-parser deps/glad/include deps/glm)

find_package(glfw3 3.3 REQUIRED)

target_link_libraries(gravity-simulation glfw)

