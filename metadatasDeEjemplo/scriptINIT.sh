#bin/bash

clonar() {
    git clone $1
}

instalarCOMMONS() {
    cd /home/utnso/workspace/so-commons-library
    sudo make install
}

instalarGUI() {
    sudo apt-get install libncurses5-dev
    cd /home/utnso/workspace/so-nivel-gui-library
    make
    sudo make install
}

instalarPKMNUTILS() {
    cd /home/utnso/workspace/so-pkmn-utils/src
    make all
    sudo make install
}

instalarSHARED() {
    cd /home/utnso/workspace/tp-2016-2c-MeQuedeSinPokebolas-/sharedCharmander/
    sudo make install
}

instalarCOMUNICACION() {
    cd /home/utnso/workspace/tp-2016-2c-MeQuedeSinPokebolas-/comunicacion/
    sudo make install
}

instalarOSADA() {
    cd /home/utnso/workspace/tp-2016-2c-MeQuedeSinPokebolas-/osadaFS/
    sudo make install
}

compilarTodo() {
	cd /home/utnso/workspace/tp-2016-2c-MeQuedeSinPokebolas-/mapa/src/
	sudo make
	
	cd /home/utnso/workspace/tp-2016-2c-MeQuedeSinPokebolas-/entrenador/src/
	sudo make

	cd /home/utnso/workspace/tp-2016-2c-MeQuedeSinPokebolas-/pokedexServidor/src/
	sudo make

	cd /home/utnso/workspace/tp-2016-2c-MeQuedeSinPokebolas-/pokedexCliente/src/
	sudo make
}

export INSTALL_DIR=/usr/lib
export INSTALL_DIR_INCLUDE=/usr/include
export MAPA_SO=mapa.so
export CHARMANDER_SO=libsharedCharmander.so
export ENTRENADOR_SO=entrenador.so
export NIVEL_SO=libcomunicacion.so

repoTP="https://github.com/sisoputnfrba/tp-2016-2c-MeQuedeSinPokebolas-.git"
repoCommons="https://github.com/sisoputnfrba/so-commons-library.git"
repoGUI="https://github.com/sisoputnfrba/so-nivel-gui-library.git"
repoBatalla="https://github.com/sisoputnfrba/so-pkmn-utils.git"
repoOSADA="https://github.com/sisoputnfrba/osada-utils.git"


cd /home/utnso/
mkdir workspace
cd /home/utnso/workspace/


clonar $repoTP
clonar $repoCommons
clonar $repoGUI
clonar $repoBatalla
clonar $repoOSADA

instalarCOMMONS
instalarGUI
instalarPKMNUTILS
instalarSHARED
instalarCOMUNICACION
instalarOSADA
compilarTodo

export SERVER_PORT=4969

cd /home/utnso/workspace/tp-2016-2c-MeQuedeSinPokebolas-/pokedexCliente/src/
mkdir tmp

cd /home/utnso/
sudo chmod -R 777 workspace/ 

cd /home/utnso/workspace/




