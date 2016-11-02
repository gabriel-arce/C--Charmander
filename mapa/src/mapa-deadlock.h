/*
 * mapa-deadlock.h
 *
 *  Created on: 31/10/2016
 *      Author: utnso
 */

#ifndef MAPA_DEADLOCK_H_
#define MAPA_DEADLOCK_H_

#include "mapa.h"

typedef struct {
	int filas;
	int columnas;
	int ** data;
} t_matriz;

int run_deadlock_algorithm();
bool puede_asignar(int x, int * sol, int * disp);
bool todos_marcados(int x, int * vec);
int cantidad_no_marcados(int x, int * vec);
int * crear_vector(int elems);
t_matriz * crear_matriz(int filas, int columnas);
void imprimir_matriz_en_log(t_matriz * matriz, char * nombre_matriz);
void mostrar_matriz(t_matriz * mat);
void mostrar_vector(int * vec, int cols);
int * crear_vector_Disponibles();
t_matriz * crear_matriz_Asignados();
t_list * snapshot_list(t_list * source_list);
int indice_en_lista_pokenests(char id);
t_matriz * crear_matriz_Solicitudes();
int * vector_copy(int * vec_src, int elems);
void marcar_sin_pkms(int * marcados);

#endif /* MAPA_DEADLOCK_H_ */
