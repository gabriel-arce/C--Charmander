POKEDEXCLIENTE_SO=pokedexCliente
INSTALL_DIR=/usr/lib
INSTALL_DIR_INCLUDE=/usr/include

 compile:
	gcc pokedexCliente.c -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=27 -o $(POKEDEXCLIENTE_SO) -l fuse -l commons -l pthread -l comunicacion 

 install: compile
	cp -u $(POKEDEXCLIENTE_SO) $(INSTALL_DIR)
	cp -u *.h $(INSTALL_DIR_INCLUDE)

 uninstall:
	rm -f $(INSTALL_DIR)/$(POKEDEXCLIENTE_SO)
	rm -f $(INSTALL_DIR_INCLUDE)/servidorPokedex.h

clean:
	rm -f $(POKEDEXCLIENTE_SO)

# PHONY targets son los que no producen un archivo llamado como el target
# Se los marca para evitar confusion si se crea un archivo llamado asi
.PHONY: compile install uninstall clean
