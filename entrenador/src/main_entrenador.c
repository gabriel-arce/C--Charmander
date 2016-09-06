/*
 * main_entrenador.c
 *
 *  Created on: 2/9/2016
 *      Author: utnso
 */

#include "entrenador.h"

int main(int argc, char ** argv) {

	socket_entrenador = -1;

	if (!chequear_argumentos(argc, TOTAL_ARGS)) {
		return EXIT_FAILURE;
	}

	leer_metadata_mapa(argv[2]);
	imprimir_metadata();

	/*
	 *  lo que tenga *** vuela despues
	 */

	int puerto_ejemplo = 9000; //***
	char * ip_ejemplo = string_duplicate("127.0.0.1"); //***

	socket_entrenador = conectarse_a_un_mapa(puerto_ejemplo, ip_ejemplo);

	if (socket_entrenador < 0) {
		return EXIT_FAILURE;
	} else {
		puts("Me conecte al mapa"); //***
	}

	srand(time(NULL));
	int random_number = rand();

	enviar_datos_a_mapa(socket_entrenador, random_number, argv[1]);

	close(socket_entrenador);
	free(ip_ejemplo);

	return EXIT_SUCCESS;
}
