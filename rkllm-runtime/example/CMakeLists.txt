cmake_minimum_required(VERSION 3.10)
project(llm_demo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Enable all compiler warnings
if (MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

# Link against pthreads under Unix systems
if (NOT MSVC AND NOT MINGW)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# Under MinGW, statically link against the standard libraries
if (MINGW)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
endif()

# Force WebSocket++ to use 100% C++11 mode, so that it doesn't try to look for Boost
# (Note that under MinGW, Boost.Thread is still required, due to a bug in MinGW that prevents the C++11 version from being used)
add_definitions(-D_WEBSOCKETPP_CPP11_STRICT_)


set(SOURCE_DIR src)
set(SOURCE_FILES src/main.cpp)
set(WEBSOCKET_SERVER_FILE src/WebsocketServer.cpp)

include_directories(SYSTEM "${SOURCE_DIR}/external")
# Compile jsoncpp from source
add_library(jsoncpp STATIC ${SOURCE_DIR}/external/jsoncpp.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${WEBSOCKET_SERVER_FILE})

set(RKLLM_API_PATH "${CMAKE_SOURCE_DIR}/../runtime/${CMAKE_SYSTEM_NAME}/librkllm_api")
include_directories(${RKLLM_API_PATH}/include)
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(RKLLM_RT_LIB ${RKLLM_API_PATH}/${CMAKE_ANDROID_ARCH_ABI}/librkllmrt.so)
    target_link_libraries(${PROJECT_NAME}  ${RKLLM_RT_LIB} log jsoncpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(RKLLM_RT_LIB ${RKLLM_API_PATH}/aarch64/librkllmrt.so)
    target_link_libraries(${PROJECT_NAME}  ${RKLLM_RT_LIB} jsoncpp)
endif()
