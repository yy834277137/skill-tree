

SRC_DIR		:= $(shell pwd)/../..
HOME_DIR	:= $(shell pwd)/../../..
CUR_DIR		:= $(shell pwd)
BASE_DIR	:= $(HOME_DIR)/src/base
MAIN_DIR    := $(HOME_DIR)/src/main
FUNC_DIR	:= $(HOME_DIR)/src/func


include $(HOME_DIR)/rules.mk

IFLAGS += -I$(HOME_DIR)/include/aud_inc
IFLAGS += -I$(HOME_DIR)/src/func/func_baseLib/pack_lib/inc
IFLAGS += -I$(HOME_DIR)/src/func/func_baseLib/unpack_lib/inc
IFLAGS += -I$(HOME_DIR)/src/func/func_baseLib/pack_unpack_inc

include $(FUNC_DIR)/rules.libs.mk



