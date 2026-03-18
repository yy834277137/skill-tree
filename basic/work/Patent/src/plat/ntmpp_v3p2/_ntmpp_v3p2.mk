#
# Makefile for ntmpp_v3p2/
#

# 头文件目录
IFLAGS += -I$(SRC_DIR)/plat/ntmpp_v3p2/SDK/hdal_33x/include
IFLAGS += -I$(SRC_DIR)/plat/ntmpp_v3p2/SDK/hdal_33x/include/vendor
IFLAGS += -I$(SRC_DIR)/plat/ntmpp_v3p2/SDK/hdal_33x/k_flow/include/kflow_videoenc
IFLAGS += -I$(SRC_DIR)/plat/ntmpp_v3p2/SDK/hdal_33x/k_flow/include/kflow_common
IFLAGS += -I$(SRC_DIR)/plat/ntmpp_v3p2/SDK/hdal_33x/k_driver/source/include
IFLAGS += -I$(SRC_DIR)/plat/ntmpp_v3p2/SDK/hdal_33x/source
IFLAGS += -I$(HOME_DIR)/include/aud_inc/hik_tts
IFLAGS += -I$(HOME_DIR)/include/aud_inc/hik_alc
IFLAGS += -I$(SRC_DIR)/plat/ntmpp_v3p2/ai/hcnn_inc_nt98336

#sys_nt.c里绑定接口需要用到vpe_nt9833x.c里的接口
IFLAGS += -I$(SRC_DIR)/plat/ntmpp_v3p2/vpe

# 宏定义
# Audio Component
#DFLAGS += -D

# 源文件目录
ALLDIRS += $(shell find $(CUR_DIR)/ntmpp_v3p2 -maxdepth 1 -type d)
ALLDIRS += $(shell find $(CUR_DIR)/ntmpp_v3p2/audio/nt -maxdepth 1 -type d)
ALLDIRS += $(shell find $(CUR_DIR)/ntmpp_v3p2/audio/tts -maxdepth 1 -type d)
ALLDIRS += $(shell find $(CUR_DIR)/ntmpp_v3p2/audio/volume -maxdepth 1 -type d)
ALLDIRS += $(shell find $(CUR_DIR)/ntmpp_v3p2/SDK/hdal_33x/vendor/isp -maxdepth 1 -type d)