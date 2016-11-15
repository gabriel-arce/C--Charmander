# Completar con sus paths

NIVEL_SO=libcomunicacion.so
INSTALL_DIR=/usr/lib
INSTALL_DIR_INCLUDE=/usr/include

compile:
	gcc -shared -fPIC -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=30 comunicacion.c -o $(NIVEL_SO)

install: compile
	cp -u $(NIVEL_SO) $(INSTALL_DIR)
	cp -u *.h $(INSTALL_DIR_INCLUDE)

uninstall:
	rm -f $(INSTALL_DIR)/$(NIVEL_SO)
	rm -f $(INSTALL_DIR_INCLUDE)/comunicacion.h

clean:
	rm -f $(NIVEL_SO)

# PHONY targets son los que no producen un archivo llamado como el target
# Se los marca para evitar confusion si se crea un archivo llamado asi
.PHONY: compile install uninstall clean
