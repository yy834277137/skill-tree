.PHONY: all clean in out

all:
	$(MAKE) -C sva	MODULE=sva  $(SVA_USE_FLAG)  $(MAKE_TARGET)
	$(MAKE) -C ba 	MODULE=ba   $(BA_USE_FLAG)   $(MAKE_TARGET)
	$(MAKE) -C ppm 	MODULE=ppm  $(PPM_USE_FLAG)  $(MAKE_TARGET)
	$(MAKE) -C face MODULE=face $(FACE_USE_FLAG)  $(MAKE_TARGET)
	
in:
	$(MAKE) -C sva	MODULE=sva  $(SVA_USE_FLAG)  $(MAKE_TARGET)
	$(MAKE) -C ba 	MODULE=ba   $(BA_USE_FLAG)   $(MAKE_TARGET)
	$(MAKE) -C ppm 	MODULE=ppm  $(PPM_USE_FLAG)  $(MAKE_TARGET)
	$(MAKE) -C face MODULE=face $(FACE_USE_FLAG)  $(MAKE_TARGET)
	
out:
	$(MAKE) -C sva 	MODULE=sva  $(SVA_USE_FLAG)  $(MAKE_TARGET)
	$(MAKE) -C ba 	MODULE=ba   $(BA_USE_FLAG)   $(MAKE_TARGET)
	$(MAKE) -C ppm 	MODULE=ppm  $(PPM_USE_FLAG)  $(MAKE_TARGET)
	$(MAKE) -C face MODULE=face $(FACE_USE_FLAG)  $(MAKE_TARGET)

clean:
	$(MAKE) -C sva  MODULE=sva  $(MAKE_TARGET)
	$(MAKE) -C ba 	MODULE=ba   $(MAKE_TARGET)
	$(MAKE) -C ppm  MODULE=ppm  $(MAKE_TARGET)
	$(MAKE) -C face MODULE=face $(MAKE_TARGET)
