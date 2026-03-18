
CUR_DIR		:= $(shell pwd)
SRC_DIR		:= $(shell pwd)/../..
HOME_DIR	:= $(shell pwd)/../../..
BASE_DIR	:= $(HOME_DIR)/src/base
MAIN_DIR    := $(HOME_DIR)/src/main
FUNC_DIR	:= $(HOME_DIR)/src/func


include $(HOME_DIR)/rules.mk

#需要用到include/jpeg_inc/目录下video_dec_common.h
IFLAGS += -I$(HOME_DIR)/include/jpeg_inc

include $(FUNC_DIR)/rules.libs.mk



