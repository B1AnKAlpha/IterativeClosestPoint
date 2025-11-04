cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(PhotoProcessing)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加可执行文件
add_executable(test_las test_las.cpp)
add_executable(icp_registration icp_registration.cpp)