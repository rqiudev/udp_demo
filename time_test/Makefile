TOPDIR=$(shell sh -c '\
        dir=`pwd|sed s/[/]$$//`; \
        while test $$dir; do \
                if test -d $$dir/.rules; then \
                        echo $$dir; \
                        exit; \
                fi; \
                dir=`echo $$dir|sed s/[/][^/]*$$//`; \
        done \
')

# 用户设定

#++++++++++++++++++++++++++++++++++++++++++#
#EXECUTABLE 是最终目标文件名，自己设定
#eg : EXECUTABLE := mushroom.exe

#MAKE_STATIC_LIB := 1
#MAKE_SHARED_LIB := 1
EXECUTABLE := test_main

#++++++++++++++++++++++++++++++++++++++++++#

#++++++++++++++++++++++++++++++++++++++++++#
#INCLUDE_PATH 是用户创建include 文件的搜索附加路径
INCLUDE_PATH := -I. -I./include -I./proto -I$(TOPDIR)/common/include

#++++++++++++++++++++++++++++++++++++++++++#

#++++++++++++++++++++++++++++++++++++++++++#
#DEFINES 用户自定义宏
DEFINES := -D__UNIX__ -D__LINUX__ -D__linux__ -D_GNU_SOURCE -DUSE_SHARED_MEM -DTEST_SHARED_MEM -std=c99
#++++++++++++++++++++++++++++++++++++++++++#

#++++++++++++++++++++++++++++++++++++++++++#
#LIBS 是一个需要连接的程序包列表（例如 alleg, stdcx, iostr 等等）`curl-config --libs`
#eg : LIBS := alleg
LIBS := -lstdc++ -lpthread -lrt `curl-config --libs`
USERLIBS := /usr/local/lib/libjson.a /usr/local/lib/libprotobuf-c.a /usr/local/libcurl/lib/libcurl.a /usr/local/lib64/libhiredis.a 
#++++++++++++++++++++++++++++++++++++++++++#

#++++++++++++++++++++++++++++++++++++++++++#
# 默认SOURCE为makefile目录的./source下的所有c/c++源文件
SOURCE :=  $(wildcard ./*.c)
_SOURCE := ./source/test.cpp

# 源文件最终集合 = [SOURCE集] - [_SOURCE集]
SOURCE := $(filter-out $(_SOURCE),$(SOURCE))

#++++++++++++++++++++++++++++++++++++++++++#

#++++++++++++++++++++++++++++++++++++++++++#
# C++预编译设置，C语言不支持预编译
PRECOMPILE_HEADER :=

#++++++++++++++++++++++++++++++++++++++++++#

-include local.$(PLATFORM).defs
include $(TOPDIR)/.rules/rules.gcc
