# Completar con sus paths

CHARMANDER_SO = libsharedCharmander.so
INSTALL_DIR=/usr/lib
INSTALL_DIR_INCLUDE=/usr/include

compile:
	gcc -shared -fPIC  shared_comunicaciones.c  shared_semaforos.c shared_configs.c shared_serializacion.c shared_sockets.c -o $(CHARMANDER_SO) -l pthread -l commons

install: compile
	cp -u $(CHARMANDER_SO) $(INSTALL_DIR)
	cp -u *.h $(INSTALL_DIR_INCLUDE)

uninstall:
	rm -f $(INSTALL_DIR)/$(CHARMANDER_SO)
	rm -f $(INSTALL_DIR_INCLUDE)/shared_comunicaciones.h
	rm -f $(INSTALL_DIR_INCLUDE)/shared_configs.h
	rm -f $(INSTALL_DIR_INCLUDE)/shared_semaforos.h
	rm -f $(INSTALL_DIR_INCLUDE)/shared_serializacion.h
	rm -f $(INSTALL_DIR_INCLUDE)/shared_sockets.h

clean:
	rm -f $(CHARMANDER_SO)

# PHONY targets son los que no producen un archivo llamado como el target
# Se los marca para evitar confusion si se crea un archivo llamado asi
.PHONY: compile install uninstall clean
