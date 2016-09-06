/*
 * main_entrenador.c
 *
 *  Created on: 2/9/2016
 *      Author: utnso
 */

#include "entrenador.h"

int main(int argc, char ** argv) {

	my_socket = -1;

	if (!chequear_argumentos(argc, 3)) {
		return EXIT_FAILURE;
	}

	/*
	 *  lo que tenga *** vuela despues
	 */

	int puerto_ejemplo = 9000; //***
	char * ip_ejemplo = string_duplicate("127.0.0.1"); //***

	my_socket = conectarse_a_un_mapa(puerto_ejemplo, ip_ejemplo);

	if (my_socket < 0) {
		return EXIT_FAILURE;
	} else {
		puts("Me conecte al mapa"); //***
	}

	srand(time(NULL));
	int random_number = rand();

	enviar_datos_a_mapa(my_socket, random_number, argv[1]);

	close(my_socket);
	free(ip_ejemplo);

	return EXIT_SUCCESS;
}
