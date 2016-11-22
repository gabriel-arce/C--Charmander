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
	pokedex_path = string_duplicate(argv[2]);

	inicializarEntrenador();
	conectarseConSiguienteMapa();

	t_header * header = NULL;

	while (!finDelJuego) {

		realizarAccion();

		if(muereEntrenador) muerteEntrenador();

		free(header);
	}

	imprimirLogro();
	finalizarEntrenador();
	liberarRecursos();

	close(socket_entrenador);

	return EXIT_SUCCESS;
}
