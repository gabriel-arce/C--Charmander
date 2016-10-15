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
#include <stdbool.h>
#include <commons/log.h>
#include <signal.h>
#include <pthread.h>
#include <shared_sockets.h>
#include <shared_comunicaciones.h>
#include <shared_semaforos.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <commons/collections/list.h>


#define NOMBRE_LOG	"ServerPokedex.log"
#define NOMBRE_PROG	"servidor-pokedex"
#define PUERTO	5000
#define IP "127.0.0.1"

typedef struct {
	int socket_cliente;
	char * directorio_montaje;
} t_sesion_cliente;

t_log *logger;
pthread_t pIDServer;
int fd_servidor;
pthread_mutex_t mutex_servidor;
bool server_on;
t_list * clientes_conectados;

void validar();
void inicializarVariables();
void crearLog();
void crearSemaforos();
void destruirSemaforos();
void crearServer();
t_sesion_cliente * crearSesionCliente(int cli_socket);
void * serverCliente(void * args);


#endif /* SERVIDOR_POKEDEX_H_ */
