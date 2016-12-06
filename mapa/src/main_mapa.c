/*
 * main_mapa.c
 *
 *  Created on: 2/9/2016
 *      Author: utnso
 */

#include "mapa.h"
#include "mapa-deadlock.h"

int main(int argc, char ** argv) {

	inicializar_variables();
	inicializar_semaforos();
	nombreMapa = string_duplicate(argv[1]);
	crear_archivo_log();

	signal(SIGUSR2, signal_handler);
	signal(SIGINT, signal_handler);

	if (chequear_argumentos(argc, TOTAL_ARGS) == -1) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Numero incorrecto de argumentos ingresados. Deben ser ./mapa.so <nombre del mapa> <punto de montaje del pkdx cliente>");
		pthread_mutex_unlock(&mutex_log);
		return EXIT_FAILURE;
	}

//	Interfaz grafica
	nivel_gui_inicializar();
	int filas = __FILAS, columnas = __COLUMNAS;
	nivel_gui_get_area_nivel(&filas, &columnas);

	ruta_directorio = string_duplicate(argv[2]);
	leer_metadata_mapa(argv[2]);
	cargar_pokenests();

	socket_servidor = -1;

//	Interfaz grafica
	nivel_gui_dibujar(items_mapa, nombreMapa);

	pthread_create(&hilo_planificador, NULL, (void *) run_scheduler_thread, NULL);
	pthread_create(&hilo_servidor, NULL, (void *) run_trainer_server, NULL);
	pthread_create(&hilo_bloqueados, NULL, (void *) atender_bloqueados, NULL);
	pthread_create(&hilo_deadlock, NULL, (void *) run_deadlock_thread, NULL);

	pthread_join(hilo_planificador, NULL);
	pthread_join(hilo_servidor, NULL);
	pthread_join(hilo_bloqueados, NULL);
	pthread_join(hilo_deadlock, NULL);

	pthread_detach(hilo_planificador);
	pthread_detach(hilo_servidor);
	pthread_detach(hilo_bloqueados);
	pthread_detach(hilo_deadlock);

	destruir_semaforos();
	destruir_variables();

	//	Interfaz grafica
	nivel_gui_terminar();

	return EXIT_SUCCESS;
}
