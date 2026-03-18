# Include Contents
include $(HOME_DIR)/rules.mk
include $(SRC_DIR)/task/AI/RULE_COMMON.MK

# Module Name
MODULE ?= DualPkgMatch

# Directory Variable
CUR_DIR  := $(shell pwd)
SRC_DIRS := $(shell find $(CUR_DIR) -maxdepth 1 -type d)
LIB_DIR ?= ./lib
OBJ_DIR ?= ./obj

# Source Files & Objectives
CPP_SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.cpp))
CPP_OBJS := $(patsubst %.cpp, $(OBJ_DIR)/%.o, $(notdir $(CPP_SRCS)))

# Header Files
MODULE_INC := $(CUR_DIR)
INC_PATH   := -I$(MODULE_INC)


# Target Info
TARGET := $(LIB_DIR)/lib$(MODULE).so

$(TARGET): $(CPP_OBJS)
	$(CPP) $(INC_PATH) $(C_FLAGS) $(OPT_FLAGS) -shared -fPIC -o $(TARGET) $(CPP_OBJS)

$(CPP_OBJS): $(OBJ_DIR)/%.o:%.cpp
	$(CPP) $(INC_PATH) $(CCFLAGS) -fPIC -o $@ -c $^
	
.PHONY: all depend clean

all:
	#@echo $(CPP) $(LIB_DIR)
	$(MAKE) depend
	$(MAKE) $(TARGET)

depend:
	#@echo $(CPP_OBJS)
	@-mkdir -p $(LIB_DIR) $(OBJ_DIR)
	
clean:
	$(RM) $(TARGET) $(CPP_OBJS)
