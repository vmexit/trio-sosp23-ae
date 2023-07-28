.PHONY: default
default: parradm

parradm: parradm.c
	gcc -o parradm parradm.c 

.PHONY: install
install:
	sudo cp -a parradm /usr/local/bin/
	sudo chown root:root /usr/local/bin/parradm
	sudo chmod 755 /usr/local/bin/parradm


.PHONY: clean
clean:
	rm -rf parradm
