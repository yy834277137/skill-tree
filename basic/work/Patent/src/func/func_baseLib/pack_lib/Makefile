
CUR_DIR		:= $(shell pwd)
SRC_DIR		:= $(shell pwd)/../../..
HOME_DIR	:= $(shell pwd)/../../../..
BASE_DIR	:= $(HOME_DIR)/src/base
MAIN_DIR    := $(HOME_DIR)/src/main
FUNC_DIR	:= $(HOME_DIR)/src/func


include $(HOME_DIR)/rules.mk

#需要用到一个公共头文件type_dscrpt_common.h, 从原先的unpack_lib目录下移到func_baseLib/pack_unpack_inc
IFLAGS += -I$(CUR_DIR)/../pack_unpack_inc

include $(FUNC_DIR)/rules.libs.mk



