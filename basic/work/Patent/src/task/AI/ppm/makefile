#include $(BASE_DIR)/build/RULE_HEADER.MK
#include $(BASE_DIR)/build/RULE_COMMON.MK

all:
	$(MAKE) -C ppm_mod_in 	MODULE=ppm   TYPE=in     $(PPM_USE_FLAG) $(MAKE_TARGET)
	$(MAKE) -C ppm_mod_out  MODULE=ppm               $(PPM_USE_FLAG) $(MAKE_TARGET)

clean:
	$(MAKE) -C ppm_mod_in   $(MAKE_TARGET)
	$(MAKE) -C ppm_mod_out  $(MAKE_TARGET)