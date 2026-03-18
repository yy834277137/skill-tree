INC_DIR := $(HOME_DIR)/include

IFLAGS += -I$(INC_DIR)
IFLAGS += -I$(INC_DIR)/aud_inc
IFLAGS += -I$(INC_DIR)/jpeg_inc
IFLAGS += -I$(INC_DIR)/open_source_inc
IFLAGS += -I$(INC_DIR)/rtspServer_inc
IFLAGS += -I$(INC_DIR)/xsp_inc

IFLAGS += -I$(INC_DIR)/ia_inc/3party/cjson_header
IFLAGS += -I$(INC_DIR)/ia_inc/3party/hcnn_header
IFLAGS += -I$(INC_DIR)/ia_inc/3party/libusb_header
IFLAGS += -I$(INC_DIR)/ia_inc/3party/vca_header

ifeq ($(plat), nt98336)
IFLAGS += -I$(INC_DIR)/ia_inc/3party/hcnn_inc_nt98336
endif

ifeq ($(plat), rk3588)
IFLAGS += -I$(INC_DIR)/ia_inc/3party/hcnn_inc_rk3588
endif