KDIR ?= ../../linux/

BDIR ?= $(CURDIR)/build
BDIR_MAKEFILE ?= $(BDIR)/Makefile
MCONFIG = CONFIG_PCI_RING_BUFFER=m

default: $(BDIR_MAKEFILE)
	$(MAKE) -C $(KDIR) M=$(BDIR) src=$(CURDIR) $(MCONFIG)

$(BDIR_MAKEFILE): $(BDIR)
	touch $(BDIR_MAKEFILE)

$(BDIR):
	mkdir -p $(BDIR)/
	mkdir -p $(BDIR)/solros_lib

install: modules_install

modules_install:
	$(MAKE) -C $(KDIR) M=$(BDIR) src=$(CURDIR) $(MCONFIG) modules_install

clean:
	$(MAKE) -C $(KDIR) M=$(BDIR) src=$(CURDIR) $(MCONFIG) clean

.PHONY: install modules_install clean
