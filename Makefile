# 生成的目标文件
TARGET:=demo_test

# 需编译的文件列表
SOURCE:=$(wildcard ./*.cpp) $(wildcard ./*.c) 

# 需要进行编译过滤的文件列表
_SOURCE:=

# 依赖的头文件路径
INCLIDE_PATH:=-I. -I./include

# 依赖的库路径
LIB_PATH:=
# 依赖的库列表
LIBS:= -lpthread -lstdc++

# 生成的目标文件类型
# 1: 静态库
# 2: 动态库
# 3: 可执行文件
TARGET_TYPE:=3


##############################################################################
##############################################################################
################################  以下为编译模块  ############################
##############################################################################
##############################################################################

CFLAG:= -Werror
SOURCE:=$(filter-out $(_SOURCE),$(SOURCE))

OBJECTS:=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCE)))
DEPS := $(patsubst %.c,%.d,$(patsubst %.cpp,%.d,$(SOURCE)))

ifeq ($(TARGET_TYPE), 1)
	TARGET:=lib$(TARGET).a
	CFLAG+= -static
else ifeq ($(TARGET_TYPE), 2)
	TARGET:=lib$(TARGET).so
	CFLAG+= -shared -fPIC
endif

ifeq ($(release), 1)
	OUTDIR=release
	CFLAG+= -O2
else
	CFLAG+= -g -O0
	OUTDIR=debug
endif

ifeq ($(gprof), 1)
	CFLAG+= -pg
	LIBS+= -lc_p
else
	LIBS += -lc
endif


.PHONY: clean print help

$(TARGET):$(OBJECTS)
ifeq ($(TARGET_TYPE), 3)
	g++ $(CFLAG) -o $@ $^ $(LIB_PATH) $(LIBS)
else ifeq ($(TARGET_TYPE), 2)
	g++ $(CFLAG) -o $@ $^ $(LIB_PATH) $(LIBS)
else ifeq ($(TARGET_TYPE), 1)
	ar crv  $@ $^ 
endif

%.o:%.cpp
	g++ $(CFLAG) -o $@ -c $< $(INCLUDE_PATH)

%.o:%.c
	gcc $(CFLAG) -o $@ -c $< $(INCLUDE_PATH)

%.d:%.c
	@set -e; rm -f $@; \
		g++ -MM $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$



%.d:%.cpp
	@set -e; rm -f $@; \
		g++ -MM $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$


-include $(DEPS)

clean:
	@-rm -f $(OBJECTS)
	@-rm -f $(TARGET)
	@-rm -f $(DEPS)

print:
	@echo -e '-------------------target------------'
	@echo $(TARGET)
	@echo -e '\n\n-----------------source------------'
	@for si in $(SOURCE);do \
		echo $$si; done \

	@echo -e '\n\n-----------------object------------'
	@for si in $(OBJECTS);do echo $$si;done

help:
	@echo this is help
