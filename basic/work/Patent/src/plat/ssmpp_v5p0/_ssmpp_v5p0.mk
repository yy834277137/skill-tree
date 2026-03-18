#
# Makefile for himpp_v5p0/
#

# 头文件目录
IFLAGS += -I$(SRC_DIR)/plat/ssmpp_v5p0/include
IFLAGS += -I$(SRC_DIR)/plat/ssmpp_v5p0/audio


# 宏定义
# Audio Component
#DFLAGS += -D

# 源文件目录
ALLDIRS += $(shell find $(SRC_DIR)/plat/ssmpp_v5p0/audio/ssv5 -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/ssmpp_v5p0/audio/ExternalCodec -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/ssmpp_v5p0/audio/Pcm -maxdepth 1 -type d)

ALLDIRS += $(shell find $(CUR_DIR)/ssmpp_v5p0 -maxdepth 1 -type d)