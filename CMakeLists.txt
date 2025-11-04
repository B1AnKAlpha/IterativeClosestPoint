cmake_minimum_required(VERSION 3.10)

# ������Ŀ����
project(PhotoProcessing)

# ���� C++ ��׼
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ���ӿ�ִ���ļ�
add_executable(test_icp test_icp.cpp)
add_executable(icp_registration icp_registration.cpp)