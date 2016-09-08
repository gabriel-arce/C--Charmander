/*
 * main_cliente.c
 *
 *  Created on: 5/9/2016
 *      Author: utnso
 */

#include "cliente-pokedex.h"

int main(int argc, char ** argv) {

	if (!chequear_argumentos(argc, TOTAL_ARGS)) {
		return EXIT_FAILURE;
	}

	directorio_montaje = string_new();
	string_append(&directorio_montaje, argv[1]);

	printf("\nCliente Pokedex en el directorio de montaje: %s\n", directorio_montaje);



	return EXIT_SUCCESS;
}
