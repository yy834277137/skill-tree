# Include Contents
CUR_DIR  := $(shell pwd)
include $(CUR_DIR)/../../../RULE_HEADER.MK
include $(CUR_DIR)/../../../RULE_COMMON.MK

# Directory Variable
SRC_DIRS := $(shell find $(CUR_DIR) -maxdepth 1 -type d)

# Source Files & Objectives
C_SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
C_OBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(C_SRCS)))

# Header Files
MODULE_INC := $(CUR_DIR)/inc
INC_PATH   := $(IFLAGS) -I$(MODULE_INC) -I$(CUR_DIR)/../inc

# Link Library Info
ifeq ($(plat), hi3559a-spc030)
LD_PATH := $(HOME_DIR)/lib/hi3559a/ia_lib/sva_lib/
LD_FLAGS := -L$(LD_PATH)
LD_FLAGS += -licf
else ifeq ($(plat), hi3519a-spc020)
#LD_PATH :=
else ifeq ($(plat), sd3403-spc003)
LD_PATH += $(HOME_DIR)/lib/sd3403/ia_lib/sva_lib/
LD_PATH += $(HOME_DIR)/lib/sd3403/ia_lib/common_lib/cnn_lib/
LD_FLAGS := -L$(LD_PATH)
LD_FLAGS += -licf
else ifeq ($(plat), ss928-spc010)
# 
else
# err
endif

# Target Info
TARGET := $(LIB_DIR)/lib$(MODULE).so

$(TARGET): $(C_OBJS)
	$(CC) $(INC_PATH) $(C_FLAGS) $(OPT_FLAGS) $(LD_FLAGS) -shared -o $(TARGET) $(C_OBJS)

$(C_OBJS): $(OBJ_DIR)/%o:%c
	$(CC) $(INC_PATH) $(CCFLAGS) -fPIC -o $@ -c $^
	
.PHONY: all depend clean debug

all:
	$(MAKE) depend
	$(MAKE) $(TARGET)

depend:
	@-mkdir -p $(LIB_DIR) $(OBJ_DIR)
	
clean:
	$(RM) $(TARGET) $(C_OBJS)
	
