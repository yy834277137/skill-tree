#*******************************************************************************
# 通用编译规则
#
# 修改方式
#
# LIB_INC        依赖库头文件
# SRC_LNC        源文件代码对外头文件
#
#******************************************************************************/
# 编译命令定义，便于修改
CC	  	:= $(CROSS)gcc
CPP     := $(CROSS)g++
AR	  	:= $(CROSS)ar
LD	  	:= $(CROSS)ld
RM	  	:= rm -rf
CP    	:= cp
STRIP 	:= $(CROSS)strip
RANLIB  := $(CROSS)ranlib
MAKE  	:= make -s


# 编译选项定义和版本宏定义
SAL_MODULE_NAME = DSP
SAL_FLAGS    := -D _GNU_SOURCE
SAL_FLAGS    += -Wall
SAL_FLAGS    += -Werror
SAL_FLAGS    += -fPIC
SAL_FLAGS    += -rdynamic
SAL_FLAGS    += -ldl
SAL_FLAGS    += -lm
SAL_FLAGS    += -lpthread
SAL_FLAGS    += -lrt

CCFLAGS      += $(DFLAGS)
C_FLAGS      := $(CCFLAGS) $(SAL_FLAGS) $(IFLAGS)

ifeq ($(CONFIG), debug)
OPT_FLAGS    := -g -D _SAL_TRACE_ 
MAKE         += -w
else
OPT_FLAGS    := -O2 -g
#OPT_FLAGS    := -O2 -g -fstack-protector-all
endif

# 动态库产生需要的定义


# 编译源文件集合
FILES_C       := $(subst ./, , $(foreach dir,.,$(wildcard $(dir)/*.c)))
FILES_CPP     := $(subst ./, , $(foreach dir,.,$(wildcard $(dir)/*.cpp)))



