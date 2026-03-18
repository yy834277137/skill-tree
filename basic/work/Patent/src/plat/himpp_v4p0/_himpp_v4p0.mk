#
# Makefile for himpp_v4p0/
#

# 头文件目录
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/include
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/adv
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/audio
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/mem
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/tde
#IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/vda
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/vgs
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/video
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/vpss
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/ive
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/dma
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/cipher
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/common
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/ai/hcnn_inc_hisi3559a
IFLAGS += -I$(SRC_DIR)/plat/himpp_v4p0/ai/hcnn_header
IFLAGS += -I$(HOME_DIR)/include/aud_inc/kdxf_tts

# 宏定义
# Audio Component
DFLAGS += -DUSE_HISI

# 源文件目录
# Audio Component
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/audio/Hisi -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/audio/ExternalCodec -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/audio/Pcm -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/audio/tts -maxdepth 1 -type d)

ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/common -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/dma -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/hardware -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/iic -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/include -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/ive -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/mem -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/osd -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/svp_dsp -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/system -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/tde -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/uart -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/util -maxdepth 1 -type d)
#ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/vda -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/vgs -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/video -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/vpss -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/cipher -maxdepth 1 -type d)
ALLDIRS += $(shell find $(SRC_DIR)/plat/himpp_v4p0/ai -maxdepth 1 -type d)