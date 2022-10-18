FILE(GLOB_RECURSE SOURCE_FILES main.cpp)
    
#相关头文件的目录    
 INCLUDE_DIRECTORIES( 
  /usr/include/c++/6.1.0 
  /usr/local/include 
  /usr/include
${PROJECT_SOURCE_DIR}/src
${PROJECT_SOURCE_DIR}/lib/libcurl-7.56.0/include
${PROJECT_SOURCE_DIR}/lib/mbedtls/include
${PROJECT_SOURCE_DIR}/lib/libwebsockets-2.4.0/include
${PROJECT_SOURCE_DIR}/lib/jsoncpp-1.8.3/include
  
)     

#所依赖库的目录路径
 LINK_DIRECTORIES(
 /usr/local/lib
 /usr/lib64
${PROJECT_BINARY_DIR}/lib
${PROJECT_SOURCE_DIR}/lib/third/linux/
${PROJECT_SOURCE_DIR}/lib/mbedtls/lib
${PROJECT_SOURCE_DIR}/lib/libwebsockets-2.4.0/lib
${PROJECT_SOURCE_DIR}/lib/jsoncpp-1.8.3/lib
 )

ADD_EXECUTABLE(utest ${SOURCE_FILES})
#依赖的库文件
                    
#设置编译的库文件存放的目录                    
TARGET_LINK_LIBRARIES(utest libjsoncpp.a)
TARGET_LINK_LIBRARIES(utest libFastAPI.a)
TARGET_LINK_LIBRARIES(utest websockets)
TARGET_LINK_LIBRARIES(utest curl)
TARGET_LINK_LIBRARIES(utest rt)
TARGET_LINK_LIBRARIES(utest pthread)
TARGET_LINK_LIBRARIES(utest dl)
TARGET_LINK_LIBRARIES(utest ssl)
TARGET_LINK_LIBRARIES(utest crypto)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

