#Author:yanglei
include(CheckCXXCompilerFlag)

cmake_minimum_required(VERSION 2.8)

#定义工程名称
PROJECT(FastAPI) 
MESSAGE(STATUS "Project Directory: ${PROJECT_SOURCE_DIR}")
SET(CMAKE_BUILE_TYPE DEBUG) 
add_definitions(-DLinux)
#指定编译器 
SET(CMAKE_CXX_FLAGS "-g -march=core2 -Wall -Wno-unused -O2 -lpthread -lm -lrt -static-libgcc")

CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11
		    support. Please use a different C++ compiler.")
endif()

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(unitTest)
