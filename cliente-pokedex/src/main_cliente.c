/*
 * main_cliente.c
 *
 *  Created on: 5/9/2016
 *      Author: utnso
 */

#include "cliente-pokedex.h"

int main(int argc, char ** argv) {

	if (chequear_argumentos(argc, TOTAL_ARGS) == -1) {
		return EXIT_FAILURE;
	}

	directorio_montaje = string_new();
	string_append(&directorio_montaje, argv[1]);

	printf("\nCliente Pokedex en el directorio de montaje: %s\n", directorio_montaje);

	socket_pokedex = conectar_con_servidor_pkdx();

	while (1) {
		char message[1000];
		scanf("%s", message);

		send(socket_pokedex, message, strlen(message), 0);
	}

	return EXIT_SUCCESS;
}
