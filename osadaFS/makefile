# Completar con sus paths

OSADA_SO=libosada.so
INSTALL_DIR=/usr/lib
INSTALL_DIR_INCLUDE=/usr/include

compile:
	gcc -shared -fPIC -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=27 osada.c -o $(OSADA_SO)

install: compile
	cp -u $(OSADA_SO) $(INSTALL_DIR)
	cp -u *.h $(INSTALL_DIR_INCLUDE)

uninstall:
	rm -f $(INSTALL_DIR)/$(OSADA_SO)
	rm -f $(INSTALL_DIR_INCLUDE)/osada.h

clean:
	rm -f $(NIVEL_SO)

# PHONY targets son los que no producen un archivo llamado como el target
# Se los marca para evitar confusion si se crea un archivo llamado asi
.PHONY: compile install uninstall clean
