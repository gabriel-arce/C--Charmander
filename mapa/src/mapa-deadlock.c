/*
 * mapa-deadlock.c
 *
 *  Created on: 22/10/2016
 *      Author: utnso
 */

#include "mapa.h"

t_list * filtrar_entrenadores_involucrados();
t_matriz * crear_matriz_Asignados(int pokenests, int entrenadores);
int * crear_vector_Disponibles();

void run_deadlock_thread() {

	while(true) {
		usleep(metadata->tiempoChequeoDeadlock);

		if (run_deadlock_algorithm() == -1) {
			pthread_mutex_lock(&mutex_log);
			log_trace(logger, "[ERROR]: El algoritmo de deteccion de deadlock devolvio un resultado de error.");
			pthread_mutex_unlock(&mutex_log);
			break;
		}
	}
}

int run_deadlock_algorithm() {

	pthread_mutex_lock(&mutex_pokenests);
	t_list * temp_pokenests = snapshot_list(lista_de_pokenests);
	pthread_mutex_unlock(&mutex_pokenests);
	pthread_mutex_lock(&mutex_entrenadores);
	t_list * temp_entrenadores = snapshot_list(entrenadores_conectados);
	pthread_mutex_unlock(&mutex_entrenadores);

	free(temp_pokenests);

	return EXIT_SUCCESS;
}

t_matriz * crear_matriz(int filas, int columnas) {
	t_matriz * matriz = malloc(sizeof(t_matriz));
	matriz->filas = filas;
	matriz->columnas = columnas;
	int f, c;

	matriz->data = (int **) malloc(filas * sizeof(int *));
	for (f = 0; f < filas; f++) {
		matriz->data[f] = (int *) malloc(columnas * sizeof(int));
	}

	//luego la inicializo
	for (f = 0; f < filas; f++) {
		for (c = 0; c < columnas; c++) {
			matriz->data[f][c] = 0;
		}
	}

	return matriz;
}

void imprimir_matriz_en_log(t_matriz * matriz, char * nombre_matriz) {
	char * mat = string_new();
	string_append_with_format(&mat, "\n***_%s_***\n", nombre_matriz);

	int i, j;

	for (i = 0; i < matriz->filas; i++) {
		string_append(&mat, "| ");
		for (j = 0; j < matriz->columnas; j++) {
			string_append_with_format(&mat, " %d ", matriz->data[i][j]);
		}
		string_append(&mat, " |\n");
	}

	log_trace(logger, mat);
}

t_matriz * crear_matriz_Asignados(int pokenests, int entrenadores) {
	t_matriz * m_asignados = crear_matriz(entrenadores, pokenests);

	return m_asignados;
}

int * crear_vector_Disponibles() {

	int pknsts = list_size(lista_de_pokenests);

	int * disponibles = malloc(pknsts * sizeof(int));

	int i = 0;

	void iterate_pokenests(t_pokenest * pknst) {
		int availables = 0;
		void iterate_available_pkm(t_pokemon * pkm) {
			if(pkm->capturado == false)
				availables++;
		}
		list_iterate(pknst->pokemones, (void *) iterate_available_pkm);

		disponibles[i] = availables;
	}
	list_iterate(lista_de_pokenests, (void *) iterate_pokenests);

	return disponibles;
}
