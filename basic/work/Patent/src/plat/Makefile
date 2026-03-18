# Makefile for plat

# 输入目录
SRC_DIR		:= $(shell pwd)/..
HOME_DIR	:= $(shell pwd)/../..
CUR_DIR		:= $(shell pwd)
BASE_DIR	:= $(HOME_DIR)/src/base
MAIN_DIR    := $(HOME_DIR)/src/main
FUNC_DIR	:= $(HOME_DIR)/src/func

# 输出目录
OBJ_DIR		:= $(CUR_DIR)/obj
LIB_DIR		:= $(CUR_DIR)/lib

# 通用编译规格
include $(HOME_DIR)/rules.mk

# 平台hal
include $(CUR_DIR)/hal/_hal.mk

# 平台封装
ifeq ($(plat), hi3559a-spc030)
include $(CUR_DIR)/himpp_v4p0/_himpp_v4p0.mk
else ifeq ($(plat), sd3403-spc003)
include $(CUR_DIR)/himpp_v5p0/_himpp_v5p0.mk
else ifeq ($(plat), ss928-spc010)
include $(CUR_DIR)/ssmpp_v5p0/_ssmpp_v5p0.mk
else ifeq ($(plat), nt98336)
include $(CUR_DIR)/ntmpp_v3p2/_ntmpp_v3p2.mk
else ifeq ($(plat), rk3588)
include $(CUR_DIR)/rkmpp_3588/_rkmpp_3588.mk
else
$(error  "Error! Error! The platform[$(plat)] which you used is unsupported!")

endif


#ALLDIRS += $(shell find $(SRC_DIR) -maxdepth 2 -type d)

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


# base库的路径
IFLAGS += -I$(BASE_DIR)/include_exter
IFLAGS += -I$(BASE_DIR)/sal
IFLAGS += -I$(BASE_DIR)/dsa
IFLAGS += -I$(BASE_DIR)/log
IFLAGS += -I$(BASE_DIR)/common
IFLAGS += -I$(BASE_DIR)/color
IFLAGS += -I$(BASE_DIR)/media

IFLAGS += -rdynamic
# dspcommon.h的路径，后续plat层需要做到不依赖dspcommon.h
IFLAGS += -I$(HOME_DIR)/include

# 需要用到main/capbility/capbility.h，后续plat层需要做到不依赖capability.h
IFLAGS += -I$(MAIN_DIR)/capbility


IFLAGS += $(foreach dir, $(ALLDIRS), -I$(dir))


# 编译输出
TARGET := libPlat.a


.PHONY: clean install debug release all dspdemo help

#编译目标，如果未指定目标，使用$(LIB_SHARE)
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	#@mkdir -p $(LIB_DIR)
	$(AR) crs -o $@ $^ 

$(C_OBJS): $(OBJ_DIR)/%.o:%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CCFLAGS) -fPIC $(DFLAGS) $(IFLAGS) -o $@ -c $^

$(CPP_OBJS): $(OBJ_DIR)/%.o:%.cpp
	#@mkdir -p $(OBJ_DIR)
	$(CPP) $(CCFLAGS) -fPIC -std=c++11 $(DFLAGS) $(IFLAGS) -c $^ -o $@


clean:
	@rm -fr $(TARGET) $(OBJ_DIR) $(LIB_DIR) 


