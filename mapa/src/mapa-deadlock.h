/*
 * mapa-deadlock.h
 *
 *  Created on: 31/10/2016
 *      Author: utnso
 */

#ifndef MAPA_DEADLOCK_H_
#define MAPA_DEADLOCK_H_

#include "mapa.h"
#include <pkmn/factory.h>
#include <pkmn/battle.h>

typedef struct {
	int filas;
	int columnas;
	int ** data;
} t_matriz;

t_list * temp_pokenests;
t_list * temp_entrenadores;
t_matriz * asignados;
t_matriz * solicitudes;
int * marcados;
int * disponibles;

t_pkmn_factory * factory;

// ******* GENERALES *******
int run_deadlock_algorithm();
void snapshot_del_sistema();
void imprimir_matriz_en_log(t_matriz * matriz, char * nombre_matriz);
void imprimir_vector_en_log(int * vector, char * nombre_vector, int rows);
void imprimir_pokenests_en_log();
void imprimir_entrenadores_en_log();
int * vector_copy(int * vec_src, int elems);

// ******* CREATES *******
t_matriz * crear_matriz(int filas, int columnas);
t_matriz * crear_matriz_Asignados();
t_matriz * crear_matriz_Solicitudes();
int * crear_vector(int elems);
int * crear_vector_Disponibles();

// ******* DESTROYERS *******
void destroy_matriz(t_matriz * matriz);
void destroy_vector(int * vector);
void release_all();

// ******* INDICES EN LISTAS *******
int indice_en_lista_pokenests(char id);
int indice_en_lista_entrenadores(char simbolo_entrenador);

// ******* FUNCIONES DEL ALGORITMO *******
void marcar_sin_pkms();
bool todos_marcados(int x, int * vec);
int cantidad_no_marcados(int x, int * vec);
bool puede_asignar(int x, int * sol, int * disp);

// ******* FUNCIONES POST ALGORITMO *******
t_list * obtener_los_dls();
t_entrenador * let_the_battle_begins();
t_entrenador * buscar_entrenador_del_pkm(t_pokemon * pkm, t_list * lista);
t_pokemon * obtener_el_mas_poronga(t_entrenador * entrenador);


#endif /* MAPA_DEADLOCK_H_ */
