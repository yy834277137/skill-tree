#
# Makefile for task
#

# 头文件
IFLAGS += -I$(SRC_DIR)/task
IFLAGS += -I$(SRC_DIR)/task/audio
IFLAGS += -I$(SRC_DIR)/task/capt
IFLAGS += -I$(SRC_DIR)/task/disp
IFLAGS += -I$(SRC_DIR)/task/dspdebug
IFLAGS += -I$(SRC_DIR)/task/dup
IFLAGS += -I$(SRC_DIR)/task/jpeg
IFLAGS += -I$(SRC_DIR)/task/link
IFLAGS += -I$(SRC_DIR)/task/osd
IFLAGS += -I$(SRC_DIR)/task/recode
IFLAGS += -I$(SRC_DIR)/task/sys
IFLAGS += -I$(SRC_DIR)/task/vca
IFLAGS += -I$(SRC_DIR)/task/vdec
IFLAGS += -I$(SRC_DIR)/task/venc

# 头文件-安检机
IFLAGS += -I$(SRC_DIR)/task/ism/xsp
IFLAGS += -I$(SRC_DIR)/task/ism/xpack
IFLAGS += -I$(SRC_DIR)/task/ism/

# 源文件目录
ALLDIRS += $(shell find $(SRC_DIR)/task -maxdepth 1 -type d)\

ifeq ($(product), $(filter $(product), ism ISM ))
ALLDIRS += $(SRC_DIR)/task/ism/xsp
ALLDIRS += $(SRC_DIR)/task/ism/xpack
endif
