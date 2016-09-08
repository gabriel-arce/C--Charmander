/*
 ============================================================================
 Name        : mapa.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "mapa.h"

void leer_metadata_mapa(char * metadata_path) {
	t_config * conf_file = config_create(metadata_path);

	metadata = malloc(sizeof(t_metadata_mapa));
	metadata->planificador = malloc(sizeof(t_planificador));

	metadata->tiempoChequeoDeadlock = getIntProperty(conf_file, "TiempoChequeoDeadlock");
	metadata->batalla = (bool) getIntProperty(conf_file, "Batalla");

	metadata->planificador->algth = getStringProperty(conf_file, "algoritmo");
	metadata->planificador->quantum = getIntProperty(conf_file, "quantum");
	metadata->planificador->retardo_turno = getIntProperty(conf_file, "retardo");

	metadata->ip = getStringProperty(conf_file, "IP");
	metadata->puerto = getIntProperty(conf_file, "Puerto");

	free(conf_file);
}

void imprimir_metada() {
	printf("\n<<<METADA DEL MAPA>>>\n");
	printf("tiempo de chequeo de DL: %d\n", metadata->tiempoChequeoDeadlock);
	printf("batalla: %d\n", metadata->batalla);
	printf("algoritmo: %s\n", metadata->planificador->algth);
	printf("quantum: %d\n", metadata->planificador->quantum);
	printf("retardo: %d\n", metadata->planificador->retardo_turno);
	printf("IP: %s\n", metadata->ip);
	printf("puerto: %d\n", metadata->puerto);
	printf("\n");
}

void crear_archivo_log() {
	logger = log_create(LOG_FILE, "MAPA log file", true, LOG_LEVEL_TRACE);
	log_info(logger, "MAPA iniciado.");
}

void inicializar_semaforos() {
	pthread_mutex_init(&mutex_servidor, 0);
}

void destruir_semaforos() {
	pthread_mutex_destroy(&mutex_servidor);
}

void inicializar_variables() {
	entrenadores_conectados = list_create();
	cola_de_listos = list_create();
	lista_de_recursos = list_create();
}

void destruir_variables() {
	list_destroy_and_destroy_elements(entrenadores_conectados, (void *) entrenador_destroyer);
	list_destroy(cola_de_listos);
	list_destroy_and_destroy_elements(lista_de_recursos, (void *) recurso_destroyer);
}

void entrenador_destroyer(t_sesion_entrenador * e) {
	free(e->nombre_entrenador);
	free(e);
}

void recurso_destroyer(t_recurso * r) {
	//TODO una vez definido el tema de los recursos y las colas de bloqueados codear el destroyer
}

void run_trainer_server() {

	void catch_error(int result) {
		if (result == -1)
			exit(EXIT_FAILURE);
	}

	int optval = 1;

	//*** TODO esto vuelva despues
	int port = 9000;
	char * ip_server = string_duplicate("127.0.0.1");
	//***

	//creo el socket de escucha
	socket_servidor = crearSocket();
	catch_error(socket_servidor);

	//bindeo a un puerto e ip
	catch_error(bindearSocket(socket_servidor, port, ip_server));

	//me pongo en escucha
	catch_error(escucharEn(socket_servidor));

	//creo la direccion del cliente
	struct sockaddr_in direccion_cliente;
	unsigned int addrlen = sizeof(struct sockaddr_in);
	int socket_nuevo = -1;

	while (true) {
		pthread_mutex_lock(&mutex_servidor);

		socket_nuevo = accept(socket_servidor, (struct sockaddr *) &direccion_cliente, &addrlen);

		if (socket_nuevo == -1) {
			printf("\n[ERROR] Error en el accept\n");
			pthread_mutex_unlock(&mutex_servidor);
			continue;
		}

		setsockopt(socket_nuevo, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

		int id = recibir_handshake(socket_nuevo);

		if (id != 1) {
			printf("\n[ERROR] Se conecto alguien desconocido\n");
			close(socket_nuevo);
			pthread_mutex_unlock(&mutex_servidor);
			continue;
		}

		t_sesion_entrenador * new_trainer_sesion = recibir_datos_entrenador(socket_nuevo);

		if (new_trainer_sesion == NULL) {
			printf("\n[ERROR] No se pudo recibir corrctamente los datos del entrenador\n");
			close(socket_nuevo);
			pthread_mutex_unlock(&mutex_servidor);
			continue;
		}

		//sacar despues...
		printf("\nSe conecto %s en el socket %d con el simbolo %c\n",
				new_trainer_sesion->nombre_entrenador,
				new_trainer_sesion->socket,
				new_trainer_sesion->simbolo_entrenador);
		//................

		list_add(entrenadores_conectados, new_trainer_sesion);

		//manda a listos al entrenador
		list_add(cola_de_listos, new_trainer_sesion);

		pthread_mutex_unlock(&mutex_servidor);
	}
}

t_sesion_entrenador * recibir_datos_entrenador(int socket_entrenador) {

	int data_buffer_size = 0;

	if (recv(socket_entrenador, &data_buffer_size, sizeof(int), 0) <= 0)
		return NULL;

	void * data_buffer = malloc(data_buffer_size);
	if (recv(socket_entrenador, data_buffer, data_buffer_size, 0) <= 0)
		return NULL;

	t_sesion_entrenador * trainer_sesion = malloc(sizeof(t_sesion_entrenador));

	trainer_sesion->socket = socket_entrenador;

	int offset = 0;
	memcpy(&(trainer_sesion->simbolo_entrenador), data_buffer + offset, sizeof(char));
	offset += sizeof(char);
	int name_size = data_buffer_size - offset;
	trainer_sesion->nombre_entrenador = malloc(sizeof(char) * name_size);
	memcpy(trainer_sesion->nombre_entrenador, data_buffer + offset, name_size);

	return trainer_sesion;
}

void run_scheduler_thread() {
	/*
	 * TODO Plantear diseño de hilos y estructuras
	 */
}

t_sesion_entrenador * buscar_entrenador_por_simbolo(char symbol_expected) {
	t_sesion_entrenador * entrenador = NULL;

	bool find_trainer(t_sesion_entrenador * s_t) {
		return (s_t->simbolo_entrenador == symbol_expected);
	}
	entrenador = list_find(entrenadores_conectados, (void *) find_trainer);

	return entrenador;
}

t_recurso * buscar_recurso_por_id(/* id */) {
	//TODO definir el recurso y su id
	t_recurso * recurso = NULL;

	return recurso;
}
