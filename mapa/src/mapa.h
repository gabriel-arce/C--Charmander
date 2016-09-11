/*
 * mapa.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef MAPA_H_
#define MAPA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shared_configs.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <sys/socket.h>
#include <shared_serializacion.h>
#include <shared_sockets.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>

#define LOG_FILE "log_mapa.log"
#define TOTAL_ARGS 3
#define PORT 9000
#define IP "127.0.0.1"

typedef struct {
	char * algth;
	int quantum;
	int retardo_turno;
} t_planificador;

typedef struct {
	int tiempoChequeoDeadlock;
	t_planificador * planificador;
	bool batalla;
	char * ip;
	int puerto;
} t_metadata_mapa;

typedef struct {
	char simbolo_entrenador;
	char * nombre_entrenador;
	int socket;
} t_sesion_entrenador;

typedef struct {
	//id del recurso
	t_list * cola_de_bloqueados;
} t_recurso;

t_metadata_mapa * metadata;
int socket_servidor;
t_log * logger;
pthread_t hilo_planificador;
pthread_t hilo_servidor;
pthread_mutex_t mutex_servidor;
t_list * entrenadores_conectados;

t_list * cola_de_listos;
t_list * lista_de_recursos; //t_recurso

//****Variables & stuff
void leer_metadata_mapa(char * metadata_path);
void imprimir_metada();
void crear_archivo_log();
void inicializar_semaforos();
void destruir_semaforos();
void inicializar_variables();
void destruir_variables();

//****Conection and threads ****
void run_trainer_server();
void run_scheduler_thread();
t_sesion_entrenador * recibir_datos_entrenador(int socket_entrenador);

//****Destroyers****
void entrenador_destroyer(t_sesion_entrenador * e);
void recurso_destroyer(t_recurso * r);

//****Buscadores****
t_sesion_entrenador * buscar_entrenador_por_simbolo(char expected_symbol);
t_recurso * buscar_recurso_por_id(/* id */);

#endif /* MAPA_H_ */
