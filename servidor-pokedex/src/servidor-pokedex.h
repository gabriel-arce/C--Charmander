/*
 * servidor-pokedex.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef SERVIDOR_POKEDEX_H_
#define SERVIDOR_POKEDEX_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <signal.h>
#include <shared_sockets.h>
#include <shared_comunicaciones.h>
#include <shared_semaforos.h>


#define NOMBRE_LOG	"ServerPokedex.log"
#define NOMBRE_PROG	"servidor-pokedex"
#define PUERTO	5000
#define IP "128.0.0.1"

void validar();
void crearLog();
void crearSemaforos();
void crearServer();

t_log *logger;
pthread_t pIDServer;

fd_set master; // conjunto maestro de descriptores de fichero
fd_set read_fds; // conjunto temporal de descriptores de fichero para select()

char buffer[2000];

#endif /* SERVIDOR_POKEDEX_H_ */
