/*
 * main_mapa.c
 *
 *  Created on: 2/9/2016
 *      Author: utnso
 */

#include "mapa.h"

int main(int argc, char ** argv) {

	if (!chequear_argumentos(argc, 3)) {
		return EXIT_FAILURE;
	}

	socket_servidor = -1;
	inicializar_variables();
	inicializar_semaforos();
	//crear_archivo_log();

	pthread_create(&hilo_servidor, NULL, (void *) run_trainer_server, NULL);
	pthread_join(hilo_servidor, NULL);
	pthread_detach(hilo_servidor);

	destruir_semaforos();
	destruir_variables();

	return EXIT_SUCCESS;
}
