.PHONY: all clean

#export SRC_DIR

all:
ifeq ($(icf_ver), 126)
	echo $(icf_ver)
	$(MAKE) -C ICF_1.2.6 	MODULE=icfcomm   all
else ifeq ($(icf_ver), 216)
	echo $(icf_ver)
	$(MAKE) -C ICF_2.1.6    MODULE=icfcomm   all
else ifeq ($(icf_ver), 218)
	echo $(icf_ver)
	$(MAKE) -C ICF_2.1.8    MODULE=icfcomm   all
endif

clean:
ifeq ($(icf_ver), 126)
	$(MAKE) -C ICF_1.2.6 	MODULE=icfcomm   clean
else ifeq ($(icf_ver), 216)
	$(MAKE) -C ICF_2.1.6    MODULE=icfcomm   clean
else ifeq ($(icf_ver), 218)
	echo $(icf_ver)
	$(MAKE) -C ICF_2.1.8    MODULE=icfcomm   clean
endif
