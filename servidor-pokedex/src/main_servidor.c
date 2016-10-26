/*
 * main_servidor.c
 *
 *  Created on: 5/9/2016
 *      Author: utnso
 */

#include "servidor-pokedex.h"

int main(int argc, char **argv) {

	printEncabezado();
	//Validar
	validar();

	//Inicializar Variables globales
	inicializarVariables();

	//Crear Log
	crearLog();

	//Crear semaforos
	crearSemaforos();


	inicializarDisco();

	//Crear servidor
	pthread_create(&pIDServer, NULL, (void *)crearServer, NULL);
	pthread_join(pIDServer, NULL);

	pthread_detach(pIDServer);


	return EXIT_SUCCESS;
}
