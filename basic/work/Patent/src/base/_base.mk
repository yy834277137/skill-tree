#
# Makefile for base
#

# 头文件
IFLAGS += -I$(SRC_DIR)/base/include_exter

IFLAGS += -I$(SRC_DIR)/base/sal
IFLAGS += -I$(SRC_DIR)/base/dsa
IFLAGS += -I$(SRC_DIR)/base/log
IFLAGS += -I$(SRC_DIR)/base/common
IFLAGS += -I$(SRC_DIR)/base/color
IFLAGS += -I$(SRC_DIR)/base/media

# 源代码
ALLDIRS += $(shell find $(SRC_DIR)/base -maxdepth 1 -type d)