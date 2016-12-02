# CMake 最低版本号要求
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# 项目信息
PROJECT(RtppTestDemo)

# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_SRCS 变量
AUX_SOURCE_DIRECTORY(. DIR_SRCS)
ADD_DEFINITIONS(-Werror)

OPTION(DEBUG_MODE "ON for debug or OFF for release" ON)
OPTION(PROFILE_MODE "ON for profile or OFF for no profile" ON)

IF(DEBUG_MODE)
    MESSAGE("This is debug mode.")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

    IF(PROFILE_MODE)
        MESSAGE("This is profile mode.")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    ENDIF()
ELSE()
    MESSAGE("This is release mode.")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")
ENDIF()


# 指定生成目标
ADD_EXECUTABLE(rtpp_rtpc_test ${DIR_SRCS})
TARGET_LINK_LIBRARIES(rtpp_rtpc_test pthread)
