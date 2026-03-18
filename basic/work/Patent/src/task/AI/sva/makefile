.PHONY: all in out clean

ifeq ($(plat),nt98336)
icf_version=218
else ifeq ($(plat),rk3588)
icf_version=218
else
icf_version=126
endif

all:
	# 临时修改：默认使用icf V1.2.6，后续将icf引擎层的代码开放给所有智能模块
	echo 111 $(plat) $(icf_version)
	
	$(MAKE) -C icf_ver icf_ver=$(icf_version) all
	$(MAKE) -C pkg_match    MODULE=DualPkgMatch     $(SVA_USE_FLAG)   all
	$(MAKE) -C sva_mod_in 	MODULE=sva   TYPE=in    $(SVA_USE_FLAG)   all
	$(MAKE) -C sva_mod_out  MODULE=sva              $(SVA_USE_FLAG)   all
in:
	$(MAKE) -C sva_mod_in 		MODULE=sva   TYPE=in    $(SVA_USE_FLAG)   all

out:
	$(MAKE) -C sva_mod_out  	MODULE=sva              $(SVA_USE_FLAG)   all

clean:
	$(MAKE) -C icf_ver icf_ver=$(icf_version) clean
	$(MAKE) -C pkg_match    MODULE=DualPkgMatch     clean
	$(MAKE) -C sva_mod_in   MODULE=sva   TYPE=in    clean
	$(MAKE) -C sva_mod_out  MODULE=sva              clean