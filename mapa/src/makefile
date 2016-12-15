# Completar con sus paths

MAPA_SO=mapa
INSTALL_DIR=/usr/lib
INSTALL_DIR_INCLUDE=/usr/include

compile:
	gcc main_mapa.c mapa.c mapa-deadlock.c mapa-utils.c -o $(MAPA_SO) -l pthread -l commons -l sharedCharmander -l nivel-gui -l curses -l pkmn-battle

 install: compile
	cp -u $(MAPA_SO) $(INSTALL_DIR)
	cp -u *.h $(INSTALL_DIR_INCLUDE)

 uninstall:
	rm -f $(INSTALL_DIR)/$(MAPA_SO)
	rm -f $(INSTALL_DIR_INCLUDE)/mapa.h
	rm -f $(INSTALL_DIR_INCLUDE)/mapa-deadlock.h

clean:
	rm -f $(MAPA_SO)

# PHONY targets son los que no producen un archivo llamado como el target
# Se los marca para evitar confusion si se crea un archivo llamado asi
.PHONY: compile install uninstall clean
