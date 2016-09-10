/*
 * main_entrenador.c
 *
 *  Created on: 2/9/2016
 *      Author: utnso
 */

#include "entrenador.h"

int main(int argc, char ** argv) {

	signal(SIGUSR1,rutina);
	signal(SIGTERM, rutina);

	if (!chequear_argumentos(argc, TOTAL_ARGS)) {
		return EXIT_FAILURE;
	}

	metadata_path = argv[2];

		inicializarEntrenador();
		conectarseConSiguienteMapa();

		while(!finDelJuego){

		esperarTurno();
		realizarAccion();
		enviarFinalizacionDeTurno();

		}

		imprimirLogro();
	
	
	return EXIT_SUCCESS;
}
