/*
 * shared_semaforos.h
 *
 *  Created on: 11/8/2016
 *      Author: utnso
 */

#ifndef SHARED_SEMAFOROS_H_
#define SHARED_SEMAFOROS_H_

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//******Semaforos comunes******
sem_t *crearSemaforo(int cantidadInicial);
void destruirSemaforo(sem_t *semaforo);
void waitSemaforo(sem_t *semaforo);
void signalSemaforo(sem_t *semaforo);

//******Semaforos mutex******
pthread_mutex_t crearMutex(); 					// **[1]**
void destruirMutex(pthread_mutex_t * mutex);
void waitMutex(pthread_mutex_t * mutex);
void signalMutex(pthread_mutex_t * mutex);

/*
 * **[1]**
 * Si el mutex esta como variable global
 * inicializarlo en el .h de la siguiente forma:
 * -- pthread_mutex_t  elMUTEX  = PTHREAD_MUTEX_INITIALIZER --
 */

#endif /* SHARED_SEMAFOROS_H_ */
