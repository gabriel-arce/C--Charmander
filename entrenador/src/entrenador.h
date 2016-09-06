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
#include <commons/collections/list.h>

#include <time.h>

#define TOTAL_ARGS 3

typedef struct {
	char * nombre;
	char simbolo;
	t_list * viaje;  //lista de t_mapa  (hoja de viaje + objetivos)
	int vidas;
	int reintentos;
} t_metadata_entrenador;

typedef struct {
	char * nombre_mapa;
	int puerto;
	char * ip;
	int socket;
	t_list * objetivos;
} t_mapa;

int socket_entrenador;
t_metadata_entrenador * metadata;

t_mapa * crear_mapa(char * nombre_mapa);
t_metadata_entrenador * crear_metadata();
int leer_metadata(char * metada_path);
void imprimir_metadata();
int conectarse_a_un_mapa(int puerto, char * ip);
int enviar_datos_a_mapa(int socket, char simbolo, char * nombre);

#endif /* ENTRENADOR_H_ */
