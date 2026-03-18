# Include Contents
include $(SRC_DIR)/task/AI/RULE_HEADER.MK
include $(SRC_DIR)/task/AI/RULE_COMMON.MK

# Directory Variable
CUR_DIR  := $(shell pwd)
SRC_DIRS := $(shell find $(CUR_DIR) -maxdepth 1 -type d)

# Source Files & Objectives
C_SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
C_OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(C_SRCS)))

# Header Files
MODULE_INC := $(CUR_DIR)/../$(MODULE)_inc
INC_PATH   := $(IFLAGS) -I$(MODULE_INC) -I$(MODULE_INC)/alg_inc

# Link Library Info
ifeq ($(plat), hi3559a-spc030)
LD_PATH := $(HOME_DIR)/lib/hi3559a/ia_lib/ppm_lib/
LD_FLAGS := -L$(LD_PATH)
LD_FLAGS += -lMtmeAlg -licf
else ifeq ($(plat), hi3519a-spc020)
#LD_PATH :=
endif

# Target Info
TARGET := $(LIB_DIR)/lib$(MODULE)$(TYPE).so

$(TARGET): $(C_OBJS)
	$(CC) $(INC_PATH) $(C_FLAGS) $(OPT_FLAGS) $(LD_FLAGS) -shared -o $(TARGET) $(C_OBJS)

$(C_OBJS): $(OBJ_DIR)/%o:%c
	$(CC) $(INC_PATH) $(C_FLAGS) -fPIC -o $@ -c $^

.PHONY: all depend clean

all:
	$(MAKE) depend
	$(MAKE) $(TARGET)

depend:
	@-mkdir -p $(LIB_DIR) $(OBJ_DIR)
	
clean:
	$(RM) $(TARGET) $(C_OBJS)
	