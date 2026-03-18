#
# Makefile for DSP Test Toolkit
#

include ../rules.mk

# 编译过程中.o文件存放的路径
ODIR = tmp

# 递归遍历各目录的2级子目录
ALLDIRS = $(shell find ./src/ -maxdepth 1 -type d)
HOME_DIR := ../
PLAT_DIR = ~/nfs/$(plat)
INSTALLDIR = $(PLAT_DIR)
RES_DIR := $(HOME_DIR)/out

vpath %.c $(ALLDIRS)
C_SRCS += $(foreach dir, $(ALLDIRS), $(wildcard $(dir)/*.c))
C_OBJS = $(patsubst %.c, $(ODIR)/%.o, $(notdir $(C_SRCS)))

vpath %.cpp $(ALLDIRS)
CPP_SRCS += $(foreach dir, $(ALLDIRS), $(wildcard $(dir)/*.cpp))
CPP_OBJS = $(patsubst %.cpp, $(ODIR)/%.o, $(notdir $(CPP_SRCS)))

CC = @echo "CC $^ --> $@"; $(CROSS)gcc
CPP = @echo "CC $^ --> $@"; $(CROSS)g++
STRIP = $(CROSS)strip
# 警告与优化选项
CCFLAGS += -Wall -Werror -O2 -g -rdynamic

################### 宏定义 ###################
DFLAGS += -D_REENTRANT -D_DEBUG

################### 头文件包含 ###################
INC_DIR += -I../include -I../src/base/sal -I../src/base/common
INC_DIR += -I./inc -I./inc/appweb -I./inc/sqlite
INC_DIR += -I./src -I./src/comm -I./src/web -I./src/sample

################### 链接库 ###################

# DSP/BSP组件库
LDFLAGS += -L$(RES_DIR)/libs -lhisdk -llog_client

# 智能相关库
LDFLAGS += -L$(RES_DIR)/libs/ppm_lib -lppm
LDFLAGS += -L$(RES_DIR)/libs/sva_lib -lsva -lDualPkgMatch -lXsieAlg
LDFLAGS += -L$(RES_DIR)/libs/ba_lib -lba

# 人脸模块动态库
LDFLAGS += -L$(RES_DIR)/libs/face_lib -lface -licf
ifeq ($(plat), rk3588)
LDFLAGS += -lSaeAlg
else ifeq ($(plat), nt98336)
LDFLAGS += -lhkann_nt_98336_encrypt -lprebuilt_ai -lvendor_ai2_pub -lvendor_ai2 -lvos
endif

# -- 上层动态库
ifeq ($(plat), rk3588)
LDFLAGS += -L$(RES_DIR)/libs/tts_lib
LDFLAGS += -lhiknlp-pinyin -lhiknlp-pinyin_rk3588_linux -lhiknlu_fs2_lib -lhiknlu_tn_rule -lhiknlu-base -lhiknlu-base-mem-matrix
LDFLAGS += -lhiknlu-mem -lhiknlu-nnet -lhikspeech-ltts -lhikspeech-matrix -lhikspeech-net -lhikspeech-tts-vocoder-tmp -lopenblas
	ifeq ($(product), $(filter $(product), ism ISM))
	LDFLAGS += -lXRay -lAiXsp -laixspd
	endif
else ifeq ($(plat), nt98336)
LDFLAGS += -L$(RES_DIR)/libs/tts_lib
LDFLAGS += -lhiknlp-pinyin -lhiknlu_fs2_lib -lhiknlu_prosody_prediction -lhiknlu_prosody_prediction_lib -lhiknlu_tn_rule -lhiknlu-base -lhiknlu-base-mem-matrix 
LDFLAGS += -lhiknlu-mem -lhiknlu-nnet -lhikspeech-ltts -lhikspeech-matrix -lhikspeech-net -lhikspeech-tts-vocoder-tmp -lopenblas
	ifeq ($(product), $(filter $(product), ism ISM))
	LDFLAGS += -lXRay
	endif
endif

# -- 芯片平台动态库
ifeq ($(plat), rk3588)
LDFLAGS += -lasound -lavcodec -lavformat -lavutil -lrockchip_mpp -lrockit -lswresample -ldrm -lrga -lcrypto_extern -lrnnnrt
else ifeq ($(plat), nt98336)
LDFLAGS += -lnt_crypto_extern -lvendor_mau
endif

# 开源库
LDFLAGS += -lharfbuzz -lfreetype -lcharset -liconv
LDFLAGS += -L./lib/$(plat)/sqlite -lsqlite3
LDFLAGS += -L./lib/$(plat)/appweb -lappweb -lhttp -lpcre -lesp -lmpr
LDFLAGS += -L./lib/$(plat)/lexbor -llexbor

# 基础C库
LDFLAGS += -ldl -lm -lpthread -lrt -lstdc++

TARGET = dspttk

all: $(TARGET)

$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(C_OBJS): $(ODIR)/%.o:%.c
	@mkdir -p $(ODIR)
	$(CC) $(CCFLAGS) $(DFLAGS) $(INC_DIR) -c $^ -o $@

$(CPP_OBJS): $(ODIR)/%.o:%.cpp
	@mkdir -p $(ODIR)
	$(CPP) $(CCFLAGS) -std=c++11 $(DFLAGS) $(INC_DIR) -c $^ -o $@

clean:
	rm -fr $(ODIR) $(TARGET)

install:
	$(info ======> You do 'make ttk install' on $(plat). Achievements redirect to '$(INSTALLDIR)'.)
	@mkdir -p $(INSTALLDIR)

# ttk执行程序及其配置
	@cp -f lib/$(plat)/start.sh lib/$(plat)/gdb $(INSTALLDIR)
	@cp -f dspttk $(INSTALLDIR)
	@cp -fr config/res/font $(INSTALLDIR)/datain
	@cp -fr config/res/yuv $(INSTALLDIR)/datain
	@cp -fr config/res/gui $(INSTALLDIR)/datain
	@cp -fr config/res/tip $(INSTALLDIR)/datain
	@cp -fr config/index.html config/appweb.conf config/assets $(INSTALLDIR)
	@cp -fr config/devcfg $(INSTALLDIR)

# ko库、动态库、平台工具、TTS库
	@cp -fr $(RES_DIR)/res/mpp $(INSTALLDIR)/
	@mkdir -p $(INSTALLDIR)/lib
	@cp -fr $(RES_DIR)/libs/* $(INSTALLDIR)/lib/
	@cp -fr $(RES_DIR)/res/tools/* $(INSTALLDIR)
	@cp -rf $(RES_DIR)/res/hik_tts $(INSTALLDIR)
	@cp -rf $(RES_DIR)/res/ia_common $(INSTALLDIR)

#安检机资源
ifeq ($(product), $(filter $(product), ism ISM))
	@cp -rf $(RES_DIR)/res/xsp $(INSTALLDIR)
endif

#资源整理，考虑主从片差异
ifeq ($(plat), rk3588)
	@cp -rf $(RES_DIR)/res/sva $(INSTALLDIR)/
	@cp -rf $(RES_DIR)/res/face $(INSTALLDIR)/
else ifeq ($(plat), nt98336)
	@cp -rf $(RES_DIR)/res/sva $(INSTALLDIR)/
endif
