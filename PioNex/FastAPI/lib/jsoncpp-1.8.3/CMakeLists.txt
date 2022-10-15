#CMake file for library FastAPI.a
#Author:yanglei
#Create:2019
#源文件
FILE(GLOB_RECURSE SOURCE_FILES src/*.cpp)   
    
#相关头文件的目录    
 INCLUDE_DIRECTORIES( 
/usr/local/include 
/usr/include 
${PROJECT_SOURCE_DIR}/lib/libcurl-7.56.0/include
${PROJECT_SOURCE_DIR}/lib/mbedtls/include
${PROJECT_SOURCE_DIR}/lib/libwebsockets-2.4.0/include
${PROJECT_SOURCE_DIR}/lib/jsoncpp-1.8.3/include
)     

#所依赖库的目录路
LINK_DIRECTORIES(
/usr/local/lib
/usr/lib64
${PROJECT_SOURCE_DIR}/lib/libcurl-7.56.0/lib
${PROJECT_SOURCE_DIR}/lib/mbedtls/lib
${PROJECT_SOURCE_DIR}/lib/libwebsockets-2.4.0/lib
${PROJECT_SOURCE_DIR}/lib/jsoncpp-1.8.3/lib
)
	      
#生成静态链接库
ADD_LIBRARY(jsoncpp ${SOURCE_FILES})
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
