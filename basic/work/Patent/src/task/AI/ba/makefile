.PHONY: all in out clean

all:
	$(MAKE) -C ba_mod_in 	MODULE=ba   TYPE=in    $(BA_USE_FLAG)   all
	$(MAKE) -C ba_mod_out  MODULE=ba              $(BA_USE_FLAG)   all

in:
	$(MAKE) -C ba_mod_in 	MODULE=ba   TYPE=in    $(BA_USE_FLAG)   all

out:
	$(MAKE) -C ba_mod_out  MODULE=ba              $(BA_USE_FLAG)   all

clean:
	$(MAKE) -C ba_mod_in   MODULE=ba   TYPE=in    clean
	$(MAKE) -C ba_mod_out  MODULE=ba              clean