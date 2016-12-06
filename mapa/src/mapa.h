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
#include <shared_comunicaciones.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <commons/collections/queue.h>
#include <semaphore.h>
#include <shared_semaforos.h>
#include <time.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <nivel.h>
#include <tad_items.h>
#include <curses.h>

#define TOTAL_ARGS 3

//INTERFACES
#define _ID_HANDSHAKE 1
#define _TURNO_CONCEDIDO 2
#define _UBICACION_POKENEST 3
#define _MOVER_XY 4
#define _CAPTURAR_PKM 5
#define _OBJETIVO_CUMPLIDO 6
#define _QUEDAN_OBJETIVOS 7
#define _BATALLA 8
#define _PKM_MAS_FUERTE 9
#define _RESULTADO_BATALLA 10
#define _DATOS_FINALES 11
#define _RESULTADO_OPERACION 12

#define __FILAS 50
#define __COLUMNAS 50

typedef enum{
	RR,
	SRDF,
} t_planificador_algthm;

typedef struct {
	t_planificador_algthm algth;
	int quantum;
	int retardo_turno;
} t_planificador;

typedef struct {
	int tiempoChequeoDeadlock;
	t_planificador * planificador;
	bool batalla;
	char * ip;
	int puerto;
	char * medallaArchivo;
} t_metadata_mapa;

typedef struct {
	int x;
	int y;
} t_posicion;

typedef struct {
	char simbolo_entrenador;
	char * nombre_entrenador;
	int socket;
	t_posicion * posicion;
	t_posicion * posicionObjetivo;
	t_list * pokemonesCapturados;
	time_t tiempoDeIngresoAlMapa;
	int deadlocksInvolucrados;
	time_t momentoBloqueado;
	double tiempoBloqueado;
	bool bloqueado;
	bool objetivo_cumplido;
	bool conoce_ubicacion;
	pthread_mutex_t mutex_entrenador;
} t_entrenador;

typedef struct {
	t_posicion * posicion;
	char identificador;
	char * nombre;
	t_list * pokemones;			//t_pokemon
	char * tipo;
} t_pokenest;

typedef struct {
	t_entrenador * entrenador;
	t_pokenest * pokenest;
} t_bloqueado;

t_list * cola_de_listos; /*t_entrenador*/
t_list * cola_de_bloqueados;  /*t_bloqueado*/
t_list * cola_de_prioridad_SRDF;
t_list * lista_de_pokenests; //t_pokenest

t_metadata_mapa * metadata;
int socket_servidor;
t_log * logger;
pthread_t hilo_planificador;
pthread_t hilo_servidor;
pthread_t hilo_bloqueados;
pthread_t hilo_deadlock;
pthread_mutex_t mutex_servidor;
pthread_mutex_t mutex_planificador_turno;
pthread_mutex_t mutex_gui;
t_list * entrenadores_conectados;
sem_t * semaforo_de_listos;
sem_t * semaforo_de_bloqueados;
pthread_mutex_t mutex_cola_listos;
pthread_mutex_t mutex_cola_bloqueados;
pthread_mutex_t mutex_entrenadores;
pthread_mutex_t mutex_cola_prioridadSRDF;
pthread_mutex_t mutex_log;
pthread_mutex_t mutex_pokenests;

char * nombreMapa;   //se setea con argumento en consola
char * ruta_directorio;

bool keep_running;
t_entrenador * entrenador_corriendo;
fd_set master_fdset;
int quantum_actual;
bool cambio_metadata;
bool quiere_atrapar;

bool finalizacionDelPrograma;

//variables de la GUI
t_list * items_mapa;


//****Variables & stuff
void leer_metadata_mapa(char * metadata_path);
void imprimir_metadata();
void crear_archivo_log();
void inicializar_semaforos();
void destruir_semaforos();
void inicializar_variables();
void destruir_variables();
void cargar_pokenests();
void cargar_medalla();
void imprimir_pokenests();
void signal_handler(int signal);
void sacar_de_listos(t_entrenador * entrenador);
void sacar_de_conectados(t_entrenador * entrenador);
void sacar_de_bloqueados(t_entrenador * entrenador);
int the_number_of_the_beast(t_pkm * beast);
void loguear_cola_de_listos();
void loguear_cola_de_bloqueados();
void imprimir_lista(t_list * l, char * tittle);
void imprimir_bloqueados();

//****Conection and threads ****
void run_trainer_server();
void run_scheduler_thread();
void run_deadlock_thread();
int procesar_nuevo_entrenador(int socket_entrenador, int buffer_size);
t_entrenador * recibir_datos_entrenador(int socket_entrenador, int buffer_size);
int desconexion_entrenador(t_entrenador * entrenador, int nbytes_recv);
void atender_bloqueados();

//****Planificador****
int run_algorithm();
int correr_rr();
int correr_srdf();
int trainer_handler(t_entrenador * entrenador);
int enviar_ubicacion_pokenest(t_entrenador * entrenador, int id_pokenest);
int avanzar_posicion_entrenador(t_entrenador * entrenador, int buffer_size);
int atrapar_pokemon(t_entrenador * entrenador);
t_entrenador * calcularSRDF();
bool entrenadorMasCercaDePokenest();
int calcularDistanciaAPokenest(t_entrenador* entrenador);
int intentar_capturar_pokemon(t_bloqueado * bloqueado);

//****Destroyers****
void entrenador_destroyer(t_entrenador * e);
void pokenest_destroyer(t_pokenest * r);
void destruir_metadata();
void item_destroyer(void * item);
void pkm_destroyer(t_pkm * p);
void bloqueado_destroyer(t_bloqueado * b);
// ....
void pokemon_remover(t_pkm * pkm, t_list * list);

//****Buscadores****
t_entrenador * buscar_entrenador_por_simbolo(char expected_symbol);
t_entrenador * buscar_entrenador_por_socket(int expected_fd);
t_pokenest * buscar_pokenest_por_id(char id);
t_pokenest * buscar_pokenest_por_ubicacion(int x, int y);
ITEM_NIVEL * buscar_item_por_id(char id);

//***Funciones***
bool esta_en_pokenest(t_entrenador * entrenador);
int procesar_objetivo_cumplido(t_entrenador * entrenador);
void ordenar_pokemons(t_list * pokemons);
int liberar_pokemons(t_entrenador * e);
int incrementar_recurso(char id_pokenest);
t_pkm * obtener_primer_no_capturado(t_pokenest * pokenest);
int generar_captura(t_entrenador * entrenador, t_pokenest * pokenest, t_pkm * pokemon);
int agregar_a_cola(t_entrenador * entrenador, t_list * cola, pthread_mutex_t mutex);
t_entrenador * pop_entrenador();
t_list * snapshot_list(t_list * source_list);
void releer_metadada();
void finalizarPrograma();

//***Envios y serializaciones***
int enviar_ruta_pkm(char * ruta, int socket);
int enviar_datos_finales_entrenador(t_entrenador * entrenador);

#endif /* MAPA_H_ */
