.PHONY: all in out clean

all:
	$(MAKE) -C face_mod_in 	MODULE=face   TYPE=in    $(FACE_USE_FLAG)   all
	$(MAKE) -C face_mod_out  MODULE=face              $(FACE_USE_FLAG)   all

in:
	$(MAKE) -C face_mod_in 	MODULE=face   TYPE=in    $(FACE_USE_FLAG)   all

out:
	$(MAKE) -C face_mod_out  MODULE=face              $(FACE_USE_FLAG)   all

clean:
	$(MAKE) -C face_mod_in   MODULE=face   TYPE=in    clean
	$(MAKE) -C face_mod_out  MODULE=face              clean