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
	char id_entrenador;
	int socket;
} t_sesion_entrenador;

pthread_t hilo_planificador;
pthread_t hilo_servidor;

void run_trainer_server();
void run_scheduler_thread();

#endif /* MAPA_H_ */
