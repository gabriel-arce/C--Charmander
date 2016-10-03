/*
 * mapa-utils.c
 *
 *  Created on: 2/10/2016
 *      Author: utnso
 */

#include "mapa.h"

void imprimir_metadata() {
	printf("\n<<<METADA DEL MAPA>>>\n");
	printf("tiempo de chequeo de DL: %d\n", metadata->tiempoChequeoDeadlock);
	printf("batalla: %d\n", metadata->batalla);

	if (metadata->planificador->algth == RR) {
		printf("algoritmo: Round Robin , con quantum: %d\n", metadata->planificador->quantum);
	} else {
		printf("algoritmo: SRDF\n");
	}

	printf("retardo: %d\n", metadata->planificador->retardo_turno);
	printf("IP: %s\n", metadata->ip);
	printf("puerto: %d\n", metadata->puerto);
	printf("\n");
}

void inicializar_semaforos() {

	pthread_mutex_init(&mutex_gui, 0);
	pthread_mutex_init(&mutex_servidor, 0);
	pthread_mutex_init(&mutex_planificador_turno, 0);
	semaforo_de_listos = crearSemaforo(0);
	if (semaforo_de_listos == NULL) {
		perror("error en la creacion de semaforos");
		exit(EXIT_FAILURE);
	}
}

void destruir_semaforos() {
	pthread_mutex_destroy(&mutex_gui);
	pthread_mutex_destroy(&mutex_servidor);
	pthread_mutex_destroy(&mutex_planificador_turno);
	destruirSemaforo(semaforo_de_listos);
}

void inicializar_variables() {
	entrenadores_conectados = list_create();
	cola_de_listos = list_create();
	lista_de_pokenests = list_create();
	quantum_actual = 0;
	keep_running = false;
	entrenador_corriendo = NULL;
	items_mapa = list_create();
}

void destruir_variables() {
	list_destroy_and_destroy_elements(entrenadores_conectados, (void *) entrenador_destroyer);
	list_destroy(cola_de_listos);
	list_destroy_and_destroy_elements(lista_de_pokenests, (void *) pokenest_destroyer);
	list_destroy_and_destroy_elements(items_mapa, (void *) item_destroyer);

	free(nombreMapa);
	free(ruta_directorio);
}

void entrenador_destroyer(t_entrenador * e) {
	free(e->nombre_entrenador);
	free(e);
}

void pokenest_destroyer(t_pokenest * r) {
	//TODO una vez definido el tema de los recursos y las colas de bloqueados codear el destroyer
}

void item_destroyer(void * item) {

	ITEM_NIVEL * i = (ITEM_NIVEL *) item;

	BorrarItem(items_mapa, i->id);
}

void pokemon_remover(t_pokemon * pkm, t_list * list) {
	int i;

	for( i = 0; i < list_size(list); i++ ) {
		t_pokemon * p = list_get(list, i);

		if (string_equals_ignore_case(p->nombreArchivo, pkm->nombreArchivo))
			break;
	}

	list_remove(list, i);
}

void imprimir_pokenests() {
	void pknst_printer(t_pokenest * pknst) {
		printf("\nPokenest : %s\n", pknst->nombre);
		printf("Ubicacion: ( %d; %d )\n", pknst->posicion->x, pknst->posicion->y);
		printf("Pokemons: \n");
		void pkm_printer(t_pokemon * pkm) {
			printf("%s\n", pkm->nombreArchivo);
		}
		list_iterate(pknst->pokemones, (void *) pkm_printer);
	}
	list_iterate(lista_de_pokenests, (void *) pknst_printer);
}

bool esta_en_pokenest(t_entrenador * entrenador) {

	return (bool) ((entrenador->posicion->x == entrenador->posicionObjetivo->x)
			&& (entrenador->posicion->y == entrenador->posicionObjetivo->y));
}

t_pokemon * recibirPokemon(int socket){

	t_header * header_in = recibir_header(socket);

	void* buffer_in = malloc(header_in->tamanio);

	if (recv(socket,buffer_in,header_in->tamanio,0) < 0) {
		free(buffer_in);
		return NULL;
	}

	t_pokemon * pokemon = deserializarPokemon(buffer_in);

	free(header_in);
	free(buffer_in);

	return pokemon;
}

t_pokemon * deserializarPokemon(void* pokemonSerializado){

	t_pokemon * pokemon = malloc(sizeof(t_pokemon));

	int nombreSize;
	int nombreArchivoSize;

	memcpy(&nombreSize, pokemonSerializado, 4);
	memcpy(&nombreArchivoSize, pokemonSerializado + 4, 4);

	pokemon->nombre = (char *) malloc(sizeof(char) * nombreSize);
	pokemon->nombreArchivo = (char *) malloc(sizeof(char) * nombreArchivoSize);

	memcpy(&pokemon->nivel, pokemonSerializado + 8, 4);
	memcpy(pokemon->nombre, pokemonSerializado + 12, nombreSize);
	memcpy(pokemon->nombreArchivo, pokemonSerializado + 12 + nombreSize,
			nombreArchivoSize);

	return pokemon;
}

t_entrenador * buscar_entrenador_por_simbolo(char symbol_expected) {
	t_entrenador * entrenador = NULL;

	bool find_trainer(t_entrenador * s_t) {
		return (s_t->simbolo_entrenador == symbol_expected);
	}
	entrenador = list_find(entrenadores_conectados, (void *) find_trainer);

	return entrenador;
}

t_pokenest * buscar_pokenest_por_id(char id) {

	t_pokenest * pokenest = NULL;

	bool find_pokenest(t_pokenest * pkn) {
		return (pkn->identificador == id);
	}
	pokenest = list_find(lista_de_pokenests, (void *) find_pokenest);

	return pokenest;
}

t_pokenest * buscar_pokenest_por_ubicacion(int x, int y) {
	t_pokenest * pknest = NULL;

	bool find_pokenest2(t_pokenest * pkn) {
		return ( (pkn->posicion->x == x)&&(pkn->posicion->y == y) );
	}
	pknest = list_find(lista_de_pokenests, (void *) find_pokenest2);

	return pknest;
}

t_entrenador * buscar_entrenador_por_socket(int expected_fd) {
	t_entrenador * entrenador = NULL;

	bool find_trainer2(t_entrenador * s_t) {
		return (s_t->socket == expected_fd);
	}
	entrenador = list_find(entrenadores_conectados, (void *) find_trainer2);

	return entrenador;
}

//**********************************************************************************************

void destruir_metadata() {
	free(metadata->ip);
	free(metadata->medallaArchivo);
	free(metadata->planificador);
	free(metadata);
}

void ordenar_pokemons(t_list * pokemons) {

}
