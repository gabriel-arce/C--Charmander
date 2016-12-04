/*
 * shared_semaforos.c
 *
 *  Created on: 11/8/2016
 *      Author: utnso
 */

#include "shared_semaforos.h"

//******* SEMAFOROS ********

sem_t *crearSemaforo(int cantidadInicial){
	sem_t *semaforo = malloc(sizeof(sem_t));
	if(sem_init(semaforo,0,cantidadInicial) == -1 ){
		return NULL;
	}
	return semaforo;
}

void destruirSemaforo(sem_t *semaforo){
	sem_destroy(semaforo);
	free(semaforo);
}

void waitSemaforo(sem_t *semaforo){
	sem_wait(semaforo);
}

void signalSemaforo(sem_t *semaforo){
	sem_post(semaforo);
}

//******* MUTEX tipo POSIX ********

pthread_mutex_t crearMutex(){
	pthread_mutex_t mutex;

	pthread_mutex_init(&mutex, 0);

	return mutex;
}

void destruirMutex(pthread_mutex_t * mutex) {
	pthread_mutex_destroy(mutex);
}

void waitMutex(pthread_mutex_t * mutex) {
	pthread_mutex_lock(mutex);
}

void signalMutex(pthread_mutex_t * mutex) {
	pthread_mutex_unlock(mutex);
}
