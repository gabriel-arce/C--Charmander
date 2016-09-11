/*
 * cliente-pokedex.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef CLIENTE_POKEDEX_H_
#define CLIENTE_POKEDEX_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <shared_configs.h>
#include <shared_sockets.h>

#define TOTAL_ARGS 2
#define PORT 9000
#define IP "127.0.0.1"

char * ip_pokedex;
int puerto_pokedex;
char * directorio_montaje;
int socket_pokedex;

int conectar_con_servidor_pkdx();

#endif /* CLIENTE_POKEDEX_H_ */
