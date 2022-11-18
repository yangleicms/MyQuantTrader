#CMake file for library BinanceMD.a
#Author:yanglei
#Create:2022
#源文件
FILE(GLOB_RECURSE SOURCE_FILES *.cpp)   
    
#相关头文件的目录    
 INCLUDE_DIRECTORIES( 
/usr/local/include 
/usr/include 
${PROJECT_SOURCE_DIR}/third/include
${PROJECT_SOURCE_DIR}/third/include/tbb/include/
${PROJECT_SOURCE_DIR}/base/common
${PROJECT_SOURCE_DIR}/base/broker
${PROJECT_SOURCE_DIR}/third/include/mbedtls/
${PROJECT_SOURCE_DIR}/third/include/tbb/include/
)     

#所依赖库的目录路
LINK_DIRECTORIES(
/usr/local/lib
/usr/lib64
		     )
	      
#生成静态链接库
ADD_LIBRARY(BinanceTD ${SOURCE_FILES})
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
