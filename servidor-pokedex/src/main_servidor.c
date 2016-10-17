/*
 * main_servidor.c
 *
 *  Created on: 5/9/2016
 *      Author: utnso
 */

#include "servidor-pokedex.h"

int main(int argc, char **argv) {


	//Validar
	validar();

	//Inicializar Variables globales
	inicializarVariables();

	//Crear Log
	crearLog();

	//Crear semaforos
	crearSemaforos();

	//TODO Mapear disco

	//Crear servidor
	pthread_create(&pIDServer, NULL, (void *)crearServer, NULL);
	pthread_join(pIDServer, NULL);

	pthread_detach(pIDServer);


	//TODO A la espera de alguna accion

	return EXIT_SUCCESS;
}
