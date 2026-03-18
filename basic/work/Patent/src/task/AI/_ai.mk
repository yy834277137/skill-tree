#
# Makefile for task
#
# AI-头文件目录
IFLAGS += -I$(SRC_DIR)/task/AI/common
IFLAGS += -I$(SRC_DIR)/task/AI/dfr
IFLAGS += -I$(SRC_DIR)/task/AI/trans

# 智能模块-外部头文件
IFLAGS += -I$(SRC_DIR)/task/AI/sva/sva_mod_out
IFLAGS += -I$(SRC_DIR)/task/AI/ba/ba_mod_out
IFLAGS += -I$(SRC_DIR)/task/AI/ppm/ppm_mod_out
IFLAGS += -I$(SRC_DIR)/task/AI/face/face_mod_out

# 源文件
ALLDIRS += $(SRC_DIR)/task/AI/common
ALLDIRS += $(SRC_DIR)/task/AI/trans