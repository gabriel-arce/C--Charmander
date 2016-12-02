/*
 * shared_comunicaciones.h
 *
 *  Created on: 12/8/2016
 *      Author: utnso
 */

#ifndef SHARED_COMUNICACIONES_H_
#define SHARED_COMUNICACIONES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <commons/string.h>
#include <sys/socket.h>
#include "shared_serializacion.h"

int enviar_header(int id, int tamanio, int socket);
t_header * recibir_header(int socket);

int enviar_handshake(int socket, int id);
int recibir_handshake(int socket);

int serializarYEnviarPokemon(int id_header, t_pkm * pokemon, int socket);
t_pkm * recibirYDeserializarPokemon(int socket, int bytes_recv);

#endif /* SHARED_COMUNICACIONES_H_ */
