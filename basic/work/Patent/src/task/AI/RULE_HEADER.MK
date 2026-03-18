#*******************************************************************************
# 通用编译规则
#
# 修改方式
#
# 增加或修改目录，需要变更头文件信息
# LIB_INC        依赖库头文件
# SRC_LNC        源文件代码对外头文件
#
#******************************************************************************/
# 编译命令定义，便于修改
include $(HOME_DIR)/rules.mk

# base库的路径
IFLAGS += -I$(BASE_DIR)/include_exter
# base内部的头文件有引用其他的base内其他的头文件
IFLAGS += -I$(BASE_DIR)/sal
IFLAGS += -I$(BASE_DIR)/dsa
IFLAGS += -I$(BASE_DIR)/log
IFLAGS += -I$(BASE_DIR)/common
IFLAGS += -I$(BASE_DIR)/color
IFLAGS += -I$(BASE_DIR)/media

# plat库的路径
IFLAGS += -I$(SRC_DIR)/plat/hal
IFLAGS += -I$(SRC_DIR)/plat/hal/hal_inc_exter

# func头文件
include $(SRC_DIR)/func/_func_headfile.mk

# include头文件
include $(HOME_DIR)/include/_include.mk

# main头文件
include $(SRC_DIR)/main/_main.mk

# task头文件
include $(SRC_DIR)/task/_task.mk
include $(SRC_DIR)/task/AI/_ai.mk

# dspcommon.h的路径，sal_time.h依赖dspcommon.h, 这个后续最好是去掉
IFLAGS += -I$(HOME_DIR)/include

# 需要用到main/capbility/capbility.h，vgs_hal.h依赖capability.h，后续plat层需要做到不依赖capability.h
IFLAGS += -I$(MAIN_DIR)/capbility