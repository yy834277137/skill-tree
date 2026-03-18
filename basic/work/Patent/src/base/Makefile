
# 输入目录
SRC_DIR		:= $(shell pwd)
HOME_DIR	:= $(shell pwd)/../..
BASE_DIR	:= $(HOME_DIR)/src/base


OBJ_DIR		:= $(SRC_DIR)/obj
LIB_DIR		:= $(SRC_DIR)/lib

include $(HOME_DIR)/rules.mk

ALLDIRS += $(shell find $(SRC_DIR) -maxdepth 1 -type d)

vpath %.c $(ALLDIRS)
C_SRCS += $(foreach dir, $(ALLDIRS), $(wildcard $(dir)/*.c))
C_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(C_SRCS)))

vpath %.cpp $(ALLDIRS)
CPP_SRCS += $(foreach dir, $(ALLDIRS), $(wildcard $(dir)/*.cpp))
CPP_OBJS = $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(CPP_SRCS)))

CC = @echo "CC $(notdir $^) --> $(notdir $@)"; $(CROSS)gcc
CPP = @echo "CC $^ --> $(notdir $@)"; $(CROSS)g++
LD = @echo "LD $(OBJ_DIR)/*.o --> $@"; $(CROSS)gcc
STRIP = $(CROSS)strip
RANLIB = $(CROSS)ranlib
MAKE = make -s


#这里需要用根目录下./include/dspcommon.h, 后续plat层需要做到不依赖dspcommon.h
IFLAGS += -I$(HOME_DIR)/include

IFLAGS += -I$(BASE_DIR)/include_exter

IFLAGS += -I$(BASE_DIR)/sal
IFLAGS += -I$(BASE_DIR)/dsa
IFLAGS += -I$(BASE_DIR)/log
IFLAGS += -I$(BASE_DIR)/common
IFLAGS += -I$(BASE_DIR)/color
IFLAGS += -I$(BASE_DIR)/media

# 编译输出
TARGET := libBase.a


.PHONY: clean install debug release all dspdemo help

#编译目标，如果未指定目标，使用$(LIB_SHARE)
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	#@mkdir -p $(LIB_DIR)
	$(AR) crs -o $@ $^ 
	#echo $(C_OBJS)
	#@mv $(TARGET) $(LIB_DIR)

$(C_OBJS): $(OBJ_DIR)/%.o:%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CCFLAGS) -fPIC $(DFLAGS) $(IFLAGS) -o $@ -c $^
	#echo $(DFLAGS) $(IFLAGS)
	
$(CPP_OBJS): $(OBJ_DIR)/%.o:%.cpp
	#@mkdir -p $(OBJ_DIR)
	$(CPP) $(CCFLAGS) -fPIC -std=c++11 $(DFLAGS) $(IFLAGS) -c $^ -o $@


clean:
	@rm -fr $(TARGET) $(OBJ_DIR) $(LIB_DIR)


