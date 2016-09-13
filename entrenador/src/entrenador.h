/*
 * entrenador.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef ENTRENADOR_H_
#define ENTRENADOR_H_

#include <stdio.h>
#include <stdlib.h>
#include <shared_configs.h>
#include <shared_sockets.h>
#include <shared_comunicaciones.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <signal.h>
#include <sys/socket.h>

#define TOTAL_ARGS 3

typedef struct {
	char * nombre;
	char simbolo;
	t_queue * viaje;  //lista de t_mapa  (hoja de viaje + objetivos)
	int vidas;
	int reintentos;
} t_metadata_entrenador;

typedef struct {
	char * nombre_mapa;
	int puerto;
	char * ip;
	int socket;
	t_queue * objetivos;
} t_mapa;

typedef struct {
	int x;
	int y;
} t_posicion;

typedef struct {
	char* nombreArchivo;
	char* nombre;
	int nivel;
} t_pokemon;


char* metadata_path;
char* nombreEntrenador;
int socket_entrenador;
t_metadata_entrenador * metadata;
t_posicion * ubicacionActual;
t_posicion * ubicacionProximaPokenest;
t_list * pokemonesCapturados;    		//t_pokemon
t_mapa * mapaActual;
int cantidadDeMuertes;
t_pokemon * pokemonMasFuerte;
bool pokenestLocalizada;
bool finDelJuego;




//------------------Funciones

t_mapa * crear_mapa(char * nombre_mapa);
t_metadata_entrenador * crear_metadata();
int leer_metadata_mapa(char * metada_path);
void imprimir_metadata();
int conectarse_a_un_mapa(int puerto, char * ip);
int enviar_datos_a_mapa(int socket, char simbolo, char * nombre);

void inicializarEntrenador();
void inicializarSinmuertesNiReintentos();
void cargarMetadata();
void conectarseConSiguienteMapa();
void solicitarUbicacionDelProximoPokenest();
void enviarSolicitudUbicacionPokenest(char id_pokemon);
void avanzarHaciaElPokenest();
void atraparPokemon();
void verificarSiQuedanObjetivosEnMapa();
void verificarNivelPokemon(t_pokemon * pokemon);
void esperarTurno();
void realizarAccion();
bool estoyEnPokenest();
void imprimirLogro();
void batallaPokemon();
void muerteEntrenador();
void desconectarseDeMapa();
void finalizarEntrenador();
void destruirHojaDeViaje();
void rutina(int signal);


#endif /* ENTRENADOR_H_ */
