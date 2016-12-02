#bin/bash

function clonar() {
    git clone $1
}

function instalarCOMMONS() {
    cd /home/utnso/workspace/so-commons-library
    make
    sudo make install
}

function instalarGUI() {
    sudo apt-get install libncurses5-dev
    cd /home/utnso/workspace/so-nivel-gui-library
    make
    sudo make isntall
}

function instalarPKMNUTILS() {
    cd /home/utnso/workspace/so-pkmn-utils/src
    make all
    sudo make install
}

function instalarREPOTP() {
    local repo= "/home/utnso/workspace/tp-2016-2c-MeQuedeSinPokebolas-"
    cd $repo
    cd /sharedCharmander
    make
    sudo make install
    cd ..
    cd /comunicacion
    make
    sudo make install
    cd ..
    cd /mapa
    make
    sudo make install
    cd ..
    cd /entrenador
    make
    sudo make install
    cd $repo
}

export INSTALL_DIR=/usr/lib
export INSTALL_DIR_INCLUDE=/usr/include
export MAPA_SO=mapa.so
export CHARMANDER_SO=libsharedCharmander.so
export ENTRENADOR_SO=entrenador.so
export NIVEL_SO=libcomunicacion.so

repoCommons="https://github.com/sisoputnfrba/so-commons-library.git"
repoGUI="https://github.com/sisoputnfrba/so-nivel-gui-library.git"
repoBatalla="https://github.com/sisoputnfrba/so-pkmn-utils.git"
repoOSADA="https://github.com/sisoputnfrba/osada-utils.git"
repoTP="https://github.com/sisoputnfrba/tp-2016-2c-MeQuedeSinPokebolas-.git"

directorioBase="/home/utnso/workspace"

mkdir $directorioBase
cd $directorioBase

clonar $repoCommons
clonar $repoGUI
clonar $repoBatalla
clonar $repoOSADA
clonar $repoTP

instalarCOMMONS
instalarGUI
instalarPKMNUTILS
instalarOSADA
isntalarREPOTP


