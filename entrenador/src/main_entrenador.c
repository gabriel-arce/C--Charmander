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

	t_header * header = NULL;

	realizarAccion();

	while (!finDelJuego) {

		header = recibir_header(socket_entrenador);
		if (header->identificador != _RESULTADO_OPERACION) {
			free(header);
			break;
		} else {
			if (header->tamanio == EXIT_FAILURE) {
				free(header);
				break;
			}
		}

		realizarAccion();

		free(header);
	}

	imprimirLogro();

	close(socket_entrenador);

	return EXIT_SUCCESS;
}
