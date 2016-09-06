/*
 * entrenador.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef ENTRENADOR_H_
#define ENTRENADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared_configs.h>
#include <shared_sockets.h>
#include <shared_comunicaciones.h>

#include <time.h>

typedef struct {
	char * nombre_mapa;
	int puerto;
	char * ip;
	int socket;
} t_mapa;

int my_socket;

int conectarse_a_un_mapa(int puerto, char * ip);
int enviar_datos_a_mapa(int socket, char simbolo, char * nombre);

#endif /* ENTRENADOR_H_ */
