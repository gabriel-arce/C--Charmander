POKEDEXSERVIDOR_SO=pokedexServidor
INSTALL_DIR=/usr/lib
INSTALL_DIR_INCLUDE=/usr/include

 compile:
	gcc servidorPokedex.c -D_FILE_OFFSET_BITS=64 -o $(POKEDEXSERVIDOR_SO) -l commons -l pthread -l osadaFS -l comunicacion  

 install: compile
	cp -u $(POKEDEXSERVIDOR_SO) $(INSTALL_DIR)
	cp -u *.h $(INSTALL_DIR_INCLUDE)

 uninstall:
	rm -f $(INSTALL_DIR)/$(POKEDEXSERVIDOR_SO)
	rm -f $(INSTALL_DIR_INCLUDE)/servidorPokedex.h

clean:
	rm -f $(POKEDEXSERVIDOR_SO)

# PHONY targets son los que no producen un archivo llamado como el target
# Se los marca para evitar confusion si se crea un archivo llamado asi
.PHONY: compile install uninstall clean
