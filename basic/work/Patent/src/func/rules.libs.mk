#
# Makefile for func, func层共用编译规则
#

#编译输出
OBJ_DIR := $(CUR_DIR)/obj
LIB_DIR := $(CUR_DIR)/lib

#编译源文件
ALLDIRS += $(shell find $(CUR_DIR) -maxdepth 1 -type d)

vpath %.c $(ALLDIRS)
C_SRCS += $(foreach dir, $(ALLDIRS), $(wildcard $(dir)/*.c))
C_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(C_SRCS)))

vpath %.cpp $(ALLDIRS)
CPP_SRCS += $(foreach dir, $(ALLDIRS), $(wildcard $(dir)/*.cpp))
CPP_OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(CPP_SRCS)))

#交叉工具链
CC = @echo "CC $(notdir $^) --> $(notdir $@)"; $(CROSS)gcc
CPP = @echo "CC $^ --> $(notdir $@)"; $(CROSS)g++
LD = @echo "LD $(OBJ_DIR)/*.o --> $@"; $(CROSS)gcc
STRIP = $(CROSS)strip
RANLIB = $(CROSS)ranlib
MAKE = make -s

# base库的路径
IFLAGS += -I$(BASE_DIR)/include_exter
# base内部的头文件有引用其他的base内其他的头文件
IFLAGS += -I$(BASE_DIR)/sal
IFLAGS += -I$(BASE_DIR)/dsa
IFLAGS += -I$(BASE_DIR)/log
IFLAGS += -I$(BASE_DIR)/common
IFLAGS += -I$(BASE_DIR)/color
IFLAGS += -I$(BASE_DIR)/media

# plat库的路径
IFLAGS += -I$(SRC_DIR)/plat/hal

# dspcommon.h的路径，sal_time.h依赖dspcommon.h, 这个后续最好是去掉
IFLAGS += -I$(HOME_DIR)/include

# 需要用到main/capbility/capbility.h，vgs_hal.h依赖capability.h，后续plat层需要做到不依赖capability.h
IFLAGS += -I$(MAIN_DIR)/capbility

# func层，各个模块自身的头文件
IFLAGS += $(foreach dir, $(ALLDIRS), -I$(dir))

# 编译输出
TARGET := lib$(module)_func.a

.PHONY: clean install debug release all dspdemo help

#编译目标，如果未指定目标，使用$(LIB_SHARE)
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	#@mkdir -p $(LIB_DIR)
	$(AR) crs -o $@ $^ 
	cp $(TARGET) $(SRC_DIR)/func/$(OUT_FUNC_LIB_DIR)/

$(C_OBJS): $(OBJ_DIR)/%.o:%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CCFLAGS) -fPIC $(DFLAGS) $(IFLAGS) -o $@ -c $^

$(CPP_OBJS): $(OBJ_DIR)/%.o:%.cpp
	#@mkdir -p $(OBJ_DIR)
	$(CPP) $(CCFLAGS) -fPIC -std=c++11 $(DFLAGS) $(IFLAGS) -c $^ -o $@

clean:
	@rm -fr $(TARGET) $(OBJ_DIR) $(LIB_DIR) 


