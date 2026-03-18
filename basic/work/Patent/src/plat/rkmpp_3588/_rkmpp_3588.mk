#
# Makefile for rkmpp_3588/
#

# 头文件
IFLAGS += -I$(SRC_DIR)/plat/rkmpp_3588/include
IFLAGS += -I$(SRC_DIR)/plat/rkmpp_3588/include/inc_rk3588_sdk
IFLAGS += -I$(HOME_DIR)/include/aud_inc/hik_tts
IFLAGS += -I$(SRC_DIR)/plat/rkmpp_3588/ai/hcnn_inc_rk3588

# 宏定义
# Audio Component
#DFLAGS += -D

# 源文件
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/audio -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/common -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/disp -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/dma -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/hardware -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/iic -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/mem -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/osd -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/system -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/tde -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/uart -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/vdec -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/venc -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/vgs -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/vi -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/vpss -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/rkmpp_3588/ai -maxdepth 1 -type d)
