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

ITEM_NIVEL * buscar_item_por_id(char id) {

	ITEM_NIVEL * item;

	bool find_item(ITEM_NIVEL * i) {
		return (i->id == id);
	}
	item = list_find(items_mapa, (void *) find_item);

	return item;
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

int liberar_pokemons(t_entrenador * e) {

	void free_pkm(t_pokemon * p) {
		p->capturado = false;
		//interfaz grafica
		//incrementar_recurso(p->id_pokenest);
	}
	list_iterate(e->pokemonesCapturados, (void *) free_pkm);

	list_clean(e->pokemonesCapturados);

	return EXIT_SUCCESS;
}

int incrementar_recurso(char id_pokenest) {

	pthread_mutex_lock(&mutex_gui);

	ITEM_NIVEL * item = buscar_item_por_id(id_pokenest);

	if (item == NULL)
		return EXIT_FAILURE;

	item->quantity++;

	pthread_mutex_unlock(&mutex_gui);

	return EXIT_SUCCESS;
}

void sacar_de_listos(t_entrenador * e) {
	int listos = list_size(cola_de_listos);
	int i;
	for (i = 0; i < listos; i++) {
		t_entrenador * e = list_get(cola_de_listos, i);

		if (e->socket == e->socket) {
			list_remove(cola_de_listos, i);
			break;
		}
	}
}

void sacar_de_conectados(t_entrenador * e) {
	int totales = list_size(entrenadores_conectados);
	int i;

	for (i = 0; i < totales; i++) {
		t_entrenador * e = list_get(entrenadores_conectados, i);

		if (e->socket == e->socket) {

			if (e == entrenador_corriendo) {
				keep_running = false;
				entrenador_corriendo = NULL;
				quantum_actual = 0;
			}

			list_remove(entrenadores_conectados, i);
			break;
		}
	}
}

void sacar_de_bloqueados(t_entrenador * e) {
	int i, j;
	int pknsts = list_size(lista_de_pokenests);
	int bloqueados = 0;
	t_pokenest * pokenest = NULL;

	for (i = 0; i < pknsts; i++) {
		pokenest = list_get(lista_de_pokenests, i);

		bloqueados = queue_size(pokenest->entrenadoresBloqueados);

		for (j = 0; j < bloqueados; j++) {
			t_entrenador * ent = list_get(pokenest->entrenadoresBloqueados->elements, j);

			if (ent->simbolo_entrenador == e->simbolo_entrenador) {
				list_remove(pokenest->entrenadoresBloqueados->elements, j);
				break;
			}
		}
	}
}

t_pokemon * obtener_primer_no_capturado(t_pokenest * pokenest) {
	t_pokemon * pkm_capt = NULL;
	int pokemons = list_size(pokenest->pokemones);
	int i;

	for(i = 0; i < pokemons; i++) {
		t_pokemon * p = list_get(pokenest->pokemones, i);

		if ( !(p->capturado) ) {
			pkm_capt = p;
			break;
		}
	}

	return pkm_capt;
}
