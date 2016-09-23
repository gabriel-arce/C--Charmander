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

	if (chequear_argumentos(argc, TOTAL_ARGS) == -1) {
		return EXIT_FAILURE;
	}

	nombreEntrenador = string_duplicate(argv[1]);
	metadata_path = string_duplicate(argv[2]);

	inicializarEntrenador();
	conectarseConSiguienteMapa();

	while (!finDelJuego) {
		esperarTurno();
		realizarAccion();
	}

	imprimirLogro();

	close(socket_entrenador);

	return EXIT_SUCCESS;
}
