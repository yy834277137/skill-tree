
CUR_DIR		:= $(shell pwd)
SRC_DIR		:= $(shell pwd)/../..
HOME_DIR	:= $(shell pwd)/../../..
BASE_DIR	:= $(HOME_DIR)/src/base
MAIN_DIR    := $(HOME_DIR)/src/main
FUNC_DIR	:= $(HOME_DIR)/src/func


include $(HOME_DIR)/rules.mk

IFLAGS += -I$(HOME_DIR)/src/func/func_baseLib/freetype
IFLAGS += -I$(HOME_DIR)/include/open_source_inc/freetype2
IFLAGS += -I$(HOME_DIR)/include/open_source_inc/freetype2/freetype
IFLAGS += -I$(MAIN_DIR)

include $(FUNC_DIR)/rules.libs.mk



