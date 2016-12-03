# Completar con sus paths

ENTRENADOR_SO=entrenador.so
INSTALL_DIR=/usr/lib
INSTALL_DIR_INCLUDE=/usr/include

compile:
	gcc main_entrenador.c entrenador.c -o $(ENTRENADOR_SO) -l commons -l sharedCharmander

 install: compile
	cp -u $(ENTRENADOR_SO) $(INSTALL_DIR)
	cp -u *.h $(INSTALL_DIR_INCLUDE)

 uninstall:
	rm -f $(INSTALL_DIR)/$(ENTRENADOR_SO)
	rm -f $(INSTALL_DIR_INCLUDE)/entrenador.h

clean:
	rm -f $(ENTRENADOR_SO)

# PHONY targets son los que no producen un archivo llamado como el target
# Se los marca para evitar confusion si se crea un archivo llamado asi
.PHONY: compile install uninstall clean
