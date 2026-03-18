#
# Makefile for hal
#

# 头文件
IFLAGS += -I$(SRC_DIR)/plat/hal
IFLAGS += -I$(SRC_DIR)/plat/hal/hal_inc_inter
IFLAGS += -I$(SRC_DIR)/plat/hal/hal_inc_exter

# 源文件
ALLDIRS += $(shell find $(SRC_DIR)/plat/hal -maxdepth 1 -type d)

