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

	/* master file descriptor list */
	fd_set master_fdset;
	/* temp file descriptor list for select() */
	fd_set read_fds;

	/* server address */
	struct sockaddr_in serveraddr;
	/* client address */
	struct sockaddr_in clientaddr;

	/* maximum file descriptor number */
	int fdmax;

	/* listening socket descriptor */
	int listener;

	/* newly accept()ed socket descriptor */
	int newfd;

	/* buffer for client data */
	char buf[1024];

	int nbytes;

	/* for setsockopt() SO_REUSEADDR, below */
	int optval = 1;
	int addrlen;
	int i, j;

	/* clear the master and temp sets */
	FD_ZERO(&master_fdset);
	FD_ZERO(&read_fds);

	/* get the listener */
	if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)	{
		perror("Server-socket() error");
		exit(1);
	}
	printf("Server-socket() is OK...\n");

	/*"address already in use" error message */
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
		perror("Server-setsockopt() error");
		exit(1);
	}
	printf("Server-setsockopt() is OK...\n");

	/* bind */
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(PORT);

	memset(&(serveraddr.sin_zero), '\0', 8);

	if (bind(listener, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {
		perror("Server-bind() error");
		exit(1);
	}
	printf("Server-bind() is OK...\n");

	/* listen */
	if (listen(listener, 10) == -1) {
		perror("Server-listen() error lol!");
		exit(1);
	}
	printf("Server-listen() is OK...\n");

	/* add the listener to the master set */
	FD_SET(listener, &master_fdset);

	/* keep track of the biggest file descriptor */
	fdmax = listener;

	/* loop for new entries */
	while(1) {

		read_fds = master_fdset;

		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("Server-select() error");
			exit(1);
		}

		/*run through the existing connections looking for data to be read*/
		for (i = 0; i <= fdmax; i++) {

			if (FD_ISSET(i, &read_fds)) {

				/* we got one... */
				if (i == listener) {

					/* handle new connections */
					addrlen = sizeof(clientaddr);

					if ((newfd = accept(listener, (struct sockaddr *) &clientaddr, (socklen_t *) &addrlen)) == -1) {
						perror("Server-accept() error");
					} else {

						printf("Server-accept() is OK...\n");

						FD_SET(newfd, &master_fdset); /* add to master set */

						if (newfd > fdmax)
							fdmax = newfd;

						printf("New connection from %s on socket %d\n",
								inet_ntoa(clientaddr.sin_addr), newfd);
					}

				} else {

					/* handle data from a client */

					if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {

						/* got error or connection closed by client */
						if (nbytes == 0)
							/* connection closed */
							printf("Socket %d disconnected\n", i);
						else
							/* just an error occurs */
							perror("recv() error");

						/* close it... */
						close(i);

						/* remove from master set */
						FD_CLR(i, &master_fdset);

					} else {
						/* we got some data from a client*/
//						for (j = 0; j <= fdmax; j++) {
//
//							/* send to everyone! */
//							if (FD_ISSET(j, &master)) {
//
//								/* except the listener and ourselves */
//								if (j != listener && j != i) {
//									if (send(j, buf, nbytes, 0) == -1)
//										perror("send() error");
//								}
//							}
//						}

					}
				}
			}
		}
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

	int simbolo = 0;

	int offset = 0;
	memcpy(&(simbolo), data_buffer + offset, sizeof(char));
	trainer_sesion->simbolo_entrenador = (char) simbolo;
	offset += sizeof(int);
	int name_size = data_buffer_size - offset;
	trainer_sesion->nombre_entrenador = malloc(sizeof(char) * name_size);
	memcpy(trainer_sesion->nombre_entrenador, data_buffer + offset, name_size);

	return trainer_sesion;
}

void run_scheduler_thread() {
	/*
	 * TODO Plantear diseño de hilos y estructuras
	 */
	int entrenadores_listos = 0;

	while (true) {

		entrenadores_listos = list_size(cola_de_listos);

		if (entrenadores_listos > 0) {
//			run_algorithm(algorithm);
		} else {
			continue;
		}
	}
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
