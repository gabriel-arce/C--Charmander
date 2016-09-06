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

typedef enum _planificador_algth {
	AP_ROUND_ROBIN = 1,
	AP_SRDF = 2
} t_planificador_algth;

typedef struct {
	t_planificador_algth algth;
	int quantum;
	int retardo_turno;
} t_planificador;

typedef struct {
	char simbolo_entrenador;
	char * nombre_entrenador;
	int socket;
} t_sesion_entrenador;

int socket_servidor;
t_log * logger;
pthread_t hilo_planificador;
pthread_t hilo_servidor;
pthread_mutex_t mutex_servidor;
t_list * entrenadores_conectados;

void crear_archivo_log();
void inicializar_semaforos();
void destruir_semaforos();
void inicializar_variables();
void destruir_variables();
void run_trainer_server();
void run_scheduler_thread();
void atiende_entrenador(void * args);
t_sesion_entrenador * recibir_datos_entrenador(int socket_entrenador);

//****Destroyers****
void entrenador_destroyer(t_sesion_entrenador * e);

//****Buscadores****
t_sesion_entrenador * buscar_entrenador_por_simbolo(char symbol_expected);

#endif /* MAPA_H_ */
