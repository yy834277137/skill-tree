MODULE_NAME = libhdal
# DIRs
KDRV_DIR = $(NVT_HDAL_DIR)/drivers/k_driver
KFLOW_DIR = $(NVT_HDAL_DIR)/drivers/k_flow
UDRV_DIR = $(NVT_HDAL_DIR)/drivers/u_driver
OUTPUT_DIR = $(NVT_HDAL_DIR)/output
# INC FLAGs
EXTRA_INCLUDE += \
	-I$(KDRV_DIR)/include \
	-I$(KDRV_DIR)/include/comm/util \
	-I$(NVT_VOS_DIR)/include \
	-I$(NVT_VOS_DIR)/drivers/include \
	-I$(NVT_HDAL_DIR)/include \
	-I$(KFLOW_DIR)/include \
	-I$(KFLOW_DIR)/include/vpd \
	-I$(KFLOW_DIR)/include/kflow_common \
	-I$(KFLOW_DIR)/include/kflow_videoenc \
	-I$(KFLOW_DIR)/include/kflow_videocapture \
	-I$(KFLOW_DIR)/include/kflow_videodec \
	-I$(KFLOW_DIR)/include/kflow_videoprocess \
	-I$(KFLOW_DIR)/include/kflow_videoosg \
	-I$(KFLOW_DIR)/include/kflow_videoout \
	-I$(KFLOW_DIR)/include/kflow_gfx \
	-I$(KFLOW_DIR)/include/kflow_audioio \
	-I$(UDRV_DIR)/include/ \
	-I$(NVT_HDAL_DIR)/source/include \
	-I$(INCLUDE_DIR)

## 321 TODO: This is to include "lcd300/lcd300_fb.h".  Remove this when LCD kdrv/kflow is done   
EXTRA_INCLUDE += \
	-I$(KDRV_DIR)/source/include


.PHONY: all clean
###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
#--------- ENVIRONMENT SETTING --------------------
WARNING	= -Wall -Wundef -Wsign-compare -Wno-missing-braces -Wstrict-prototypes -Werror
COMPILE_OPTS	= -I. -O2 -fPIC -ffunction-sections -fdata-sections -D__LINUX_USER__ -D__LINUX -D__FPGA_TEST -DEXTRA_ENC_OSG -DSEND_DEC_BLACK_BS=0
C_CFLAGS	= $(NVT_GCOV) $(PLATFORM_CFLAGS) $(COMPILE_OPTS) $(ASAN_CFLAGS) $(WARNING) $(EXTRA_INCLUDE)
LD_FLAGS	= $(NVT_GCOV) -fPIC -shared -lpthread
#--------- END OF ENVIRONMENT SETTING -------------
SRC += hd_debug.c hd_logger.c hd_util.c hd_common.c
SRC += hd_videocapture.c hd_videoprocess.c hd_videodec.c hd_videoout.c hd_videoenc.c
SRC += hd_audiocap.c hd_audioout.c hd_audioenc.c hd_audiodec.c hd_gfx.c
SRC += bind.c videoout.c videocap.c videoenc.c videodec.c videoprocess.c gfx.c 
SRC += audiocap.c audioenc.c audiodec.c audioout.c
SRC += pif.c cif_common.c cif_list.c gmlib_proc.c utl.c pif_ioctl.c dif.c
SRC += hd_int.c hd_videocapture_menu.c hd_videoprocess_menu.c hd_videoout_menu.c hd_videoenc_menu.c hd_videodec_menu.c
SRC += hd_audiocap_menu.c hd_audioenc_menu.c hd_audiodec_menu.c hd_audioout_menu.c hd_util_menu.c
SRC += videocap_convert.c videoproc_convert.c videoout_convert.c videoenc_convert.c videodec_convert.c
SRC += audiocap_convert.c audioout_convert.c audioenc_convert.c audiodec_convert.c constant_data.c



OBJ = $(SRC:.c=.o)

ifeq ("$(wildcard *.c */*.c)","")
all:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
clean:
	@echo "nothing to be done for '$(OUTPUT_NAME)'"
else
all: $(OBJ)
	@echo Creating library $(MODULE_NAME) ...
	@$(CC) -o $(MODULE_NAME).so $(OBJ) $(LD_FLAGS)
	@$(AR) rcs $(MODULE_NAME).a $(OBJ)

%.o:%.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) $< -c -o $@

clean:
	@rm -f $(OBJ) *.o *.a *.so*
	@rm -rf $(OUTPUT_DIR)
endif

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf *.so* *.a  $(OUTPUT_DIR)


###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)
#--------- ENVIRONMENT SETTING --------------------
# DIRs
RTOS_KERNEL_DIR = $(KERNELDIR)/lib/FreeRTOS
RTOS_LIB_DIR = $(KERNELDIR)/lib
RTOS_CURR_DEMO_DIR = $(KERNELDIR)/demos/novatek/na51068
RTOS_POSIX_DIR = $(KERNELDIR)/lib/FreeRTOS-Plus-POSIX
RTOS_POSIX_SRC_DIR = $(RTOS_POSIX_DIR)/source

#INCs for C_CFLAGS

EXTRA_INCLUDE += \
	-I$(RTOS_LIB_DIR) \
	-I$(RTOS_KERNEL_DIR)/portable/GCC/ARM_CA9  \
	-I$(RTOS_KERNEL_DIR)/include \
	-I$(RTOS_KERNEL_DIR)/include/private \
	-I$(RTOS_CURR_DEMO_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include \
	-I$(RTOS_POSIX_DIR)/include/portable/novatek

C_CFLAGS = $(PLATFORM_CFLAGS) $(EXTRA_INCLUDE)
#--------- END OF ENVIRONMENT SETTING -------------
LIB_NAME = $(MODULE_NAME).a

SRC = \

OBJ = $(SRC:.c=.o)

all: $(LIB_NAME)

$(LIB_NAME): $(OBJ)
	@echo Creating $@...
	@$(AR) rcsD $@ $(OBJ)
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --arc-sha1 $@

%.o: %.c
	@echo Compiling $<
	@$(CC) $(C_CFLAGS) -c $< -o $@

clean:
	@rm -f $(LIB_NAME) $(OBJ) $(LIB_NAME).sym *.o *.a *.so*
	@rm -rf $(OUTPUT_DIR)

install:
	@mkdir -p $(OUTPUT_DIR)
	@cp -avf $(LIB_NAME) $(OUTPUT_DIR)
endif
