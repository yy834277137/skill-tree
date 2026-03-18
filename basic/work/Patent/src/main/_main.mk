#
# Makefile for apps
#

# 头文件目录
IFLAGS += -I$(SRC_DIR)/main
IFLAGS += -I$(SRC_DIR)/main/capbility
IFLAGS += -I$(SRC_DIR)/main/hostcmd_proc
ifeq ($(product), $(filter $(product), ism ISM ))
IFLAGS += -I$(SRC_DIR)/main/hostcmd_proc/ism
endif

# 源文件
ALLDIRS += $(SRC_DIR)/main
ALLDIRS += $(SRC_DIR)/main/capbility
ALLDIRS += $(SRC_DIR)/main/hostcmd_proc

ifeq ($(product), $(filter $(product), ism ISM ))
ALLDIRS += $(SRC_DIR)/main/hostcmd_proc/ism
endif
