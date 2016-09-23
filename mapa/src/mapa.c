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

	char * ruta = string_duplicate(metadata_path);
	string_append(&ruta, "Mapas/");
	string_append(&ruta, nombreMapa);
	string_append(&ruta, "/metadata.conf");

	t_config * conf_file = config_create(ruta);

	metadata = malloc(sizeof(t_metadata_mapa));
	metadata->planificador = malloc(sizeof(t_planificador));

	metadata->tiempoChequeoDeadlock = getIntProperty(conf_file, "TiempoChequeoDeadlock");
	metadata->batalla = (bool) getIntProperty(conf_file, "Batalla");

	char * algoritmo = getStringProperty(conf_file, "algoritmo");

	if (string_equals_ignore_case(algoritmo, "RR")) {
		metadata->planificador->algth = RR;
	} else {
		if (string_equals_ignore_case(algoritmo, "SRDF")) {
			metadata->planificador->algth = SRDF;
		} else {
			perror("Algoritmo no reconocido");
			exit(EXIT_FAILURE);
		}
	}

	metadata->planificador->quantum = getIntProperty(conf_file, "quantum");
	metadata->planificador->retardo_turno = getIntProperty(conf_file, "retardo");

	metadata->ip = getStringProperty(conf_file, "IP");
	metadata->puerto = getIntProperty(conf_file, "Puerto");

	free(conf_file);
	free(ruta);
}

void imprimir_metada() {
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

void crear_archivo_log() {
	logger = log_create(LOG_FILE, "MAPA log file", true, LOG_LEVEL_TRACE);
	log_info(logger, "MAPA iniciado.");
}

void inicializar_semaforos() {

	pthread_mutex_init(&mutex_servidor, 0);
	pthread_mutex_init(&mutex_planificador_turno, 0);
	semaforo_de_listos = crearSemaforo(0);
	if (semaforo_de_listos == NULL) {
		perror("error en la creacion de semaforos");
		exit(EXIT_FAILURE);
	}
}

void destruir_semaforos() {
	pthread_mutex_destroy(&mutex_servidor);
	pthread_mutex_destroy(&mutex_planificador_turno);
	destruirSemaforo(semaforo_de_listos);
}

void inicializar_variables() {
	entrenadores_conectados = list_create();
	cola_de_listos = list_create();
	lista_de_pokenests = list_create();
}

void destruir_variables() {
	list_destroy_and_destroy_elements(entrenadores_conectados, (void *) entrenador_destroyer);
	list_destroy(cola_de_listos);
	list_destroy_and_destroy_elements(lista_de_pokenests, (void *) pokenest_destroyer);
}

void entrenador_destroyer(t_sesion_entrenador * e) {
	free(e->nombre_entrenador);
	free(e);
}

void pokenest_destroyer(t_pokenest * r) {
	//TODO una vez definido el tema de los recursos y las colas de bloqueados codear el destroyer
}

void cargar_pokenests() {
	char * dir_pokenests = string_new();
	string_append(&dir_pokenests, ruta_directorio);
	string_append(&dir_pokenests, "Mapas/");
	string_append(&dir_pokenests, nombreMapa);
	string_append(&dir_pokenests, "/PokeNests");

	DIR * d = opendir(dir_pokenests);

	if (!d) {
		fprintf(stderr, "Cannot open directory '%s': %s\n", dir_pokenests,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	// iterate the pokenest folders
	while (1) {
		struct dirent * entry;
		const char * d_name;

		entry = readdir(d);
		if (!entry)
			break;

		d_name = entry->d_name;

		if (entry->d_type & DT_DIR) {

			if (strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0) {
				int path_length;
				char path[PATH_MAX];

				path_length = snprintf(path, PATH_MAX, "%s/%s", dir_pokenests,
						d_name);
				if (path_length >= PATH_MAX) {
					fprintf(stderr, "Path length has got too long.\n");
					exit(EXIT_FAILURE);
				}

				//now create the logical pokenest
				t_pokenest * pknst = malloc(sizeof(t_pokenest));
				pknst->nombre = string_duplicate(d_name);
				pknst->entrenadoresBloqueados = queue_create();
				pknst->pokemones = list_create();
				pknst->posicion = malloc(sizeof(t_posicion));

				//read its files
				DIR * d_pknst = opendir(path);
				while(1) {
					struct dirent * f_pknst = readdir(d_pknst);

					if (!f_pknst)
						break;

					if ( (f_pknst->d_type & DT_DIR ) || (strcmp(f_pknst->d_name, "..") == 0) || (strcmp(f_pknst->d_name, ".") == 0) )
						continue;

					char path_f_pknst[PATH_MAX];

					snprintf(path_f_pknst, PATH_MAX, "%s/%s", path, f_pknst->d_name);

					// if its the metadata go on
					if ( string_equals_ignore_case(f_pknst->d_name, "metadata") ) {

						t_config * m_pknst = config_create(path_f_pknst);

						char * id = getStringProperty(m_pknst, "Identificador");
						pknst->identificador = (char) id[0];

						char * posicion = getStringProperty(m_pknst, "Posicion");
						char ** _x_y = string_split(posicion, ";");

						pknst->posicion->x = atoi( _x_y[0] );
						pknst->posicion->y = atoi( _x_y[1] );

						pknst->tipo = getStringProperty(m_pknst, "Tipo");

						free(posicion);
						free(_x_y[0]);
						free(_x_y[1]);
						free(_x_y);
						free(m_pknst);

						continue;
					}

					// else its a pokemon
					t_config * dat_pkm = config_create(path_f_pknst);

					t_pokemon * pkm = malloc(sizeof(t_pokemon));
					pkm->nombre = string_duplicate(d_name);
					pkm->nombreArchivo = string_duplicate(f_pknst->d_name);
					pkm->nivel = getIntProperty(dat_pkm, "Nivel");

					list_add(pknst->pokemones, pkm);

					free(dat_pkm);
				}

				list_add(lista_de_pokenests, pknst);
			}
		}
	}

	/* Release everything. */
	if (closedir(d)) {
		fprintf(stderr, "Could not close '%s': %s\n", dir_pokenests,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	free(dir_pokenests);
}

// ****************************************************************************************************

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

	/* for setsockopt() SO_REUSEADDR, below */
	int optval = 1;
	int addrlen;
	int i;

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
					trainer_handler(i, &master_fdset);
				}
			}
		}
	}
}

void trainer_handler(int socket, fd_set * fdset) {
	pthread_mutex_lock(&mutex_servidor);

	int buffer_size = sizeof(t_header);
	void * buffer_in = malloc(buffer_size);

	int nbytes_recv = recv(socket, buffer_in, buffer_size, 0);

	//DESCONEXION DE UN ENTRENADOR
	if (nbytes_recv <= 0) {
		if (nbytes_recv == 0)
			printf("Socket %d disconnected\n", socket);

		if (nbytes_recv < 0)
			printf("recv error\n");

		close(socket);
		FD_CLR(socket, fdset);
		pthread_mutex_unlock(&mutex_servidor);
	}

	t_header * header = deserializar_header(buffer_in);

	//OPERACIONES ENTRENADOR
	switch (header->identificador) {
		case _ID_HANDSHAKE:
			procesar_nuevo_entrenador(socket, header->tamanio);
			break;
		case _UBICACION_POKENEST:
			puts("Ubicacion pokenest");
			enviar_ubicacion_pokenest(socket, header->tamanio);
			break;
		case _MOVER_XY:
			puts("Mover entrenador");
			avanzar_posicion_entrenador(socket, header->tamanio);
			break;
		case _CAPTURAR_PKM:
			puts("Atrapar pokemon");
			atrapar_pokemon(socket);
			break;
		default:
			break;
	}

	pthread_mutex_unlock(&mutex_servidor);
}

int procesar_nuevo_entrenador(int socket_entrenador, int buffer_size) {
	t_sesion_entrenador * nuevo_entrenador = recibir_datos_entrenador(socket_entrenador, buffer_size);

	if (nuevo_entrenador == NULL)
		return -1;

	printf("\nSe conecto %s en el socket %d con el simbolo %c\n",
			nuevo_entrenador->nombre_entrenador, nuevo_entrenador->socket,
			nuevo_entrenador->simbolo_entrenador);

	list_add(entrenadores_conectados, nuevo_entrenador);

	//manda a listos al entrenador
	list_add(cola_de_listos, nuevo_entrenador);

	signalSemaforo(semaforo_de_listos);

	return 0;
}

t_sesion_entrenador * recibir_datos_entrenador(int socket_entrenador, int data_buffer_size) {

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

	free(data_buffer);

	trainer_sesion->bloqueado = false;
	trainer_sesion->deadlocksInvolucrados = 0;
	trainer_sesion->pokemonesCapturados = list_create();
	trainer_sesion->posicion = malloc(sizeof(t_posicion));
	trainer_sesion->posicion->x = 0;
	trainer_sesion->posicion->y = 0;
	trainer_sesion->posicionObjetivo = malloc(sizeof(t_posicion));
	trainer_sesion->posicionObjetivo->x = 0;
	trainer_sesion->posicionObjetivo->y = 0;
	trainer_sesion->momentoBloqueado = 0;
	trainer_sesion->tiempoBloqueado = 0;
	time(&(trainer_sesion->tiempoDeIngresoAlMapa));
	trainer_sesion->objetivo_cumplido = false;

	return trainer_sesion;
}

void run_scheduler_thread() {

	while (true) {
		waitSemaforo(semaforo_de_listos);  //Hacer un signal siempre y cuando vuelva a la cola de listos y no se bloquee
		//corro el algoritmo
		//TODO catchear el resultado de run_algorithm
		if (run_algorithm() == EXIT_FAILURE)
			break;
	}
}

int run_algorithm() {

	int result = -1;

	switch (metadata->planificador->algth) {
	case RR:
		result = correr_rr();
		break;
	case SRDF:
		result = correr_srdf();
		break;
	default:
		break;
	}

	return result;
}

int solicitar_turno(t_sesion_entrenador * entrenador) {

	if (entrenador->bloqueado)
		return EXIT_FAILURE;

	pthread_mutex_lock(&mutex_planificador_turno);

	if (enviar_header(_TURNO_CONCEDIDO, 0, entrenador->socket) == -1)
		return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

int correr_rr() {

	int quantum = metadata->planificador->quantum;

	t_sesion_entrenador * entrenador_listo = list_get(cola_de_listos, 0);

	if (entrenador_listo == NULL)
		return EXIT_FAILURE;

	list_remove(cola_de_listos, 0);

	while ( (quantum != 0) && (!(entrenador_listo->bloqueado)) && (!(entrenador_listo->objetivo_cumplido)) ) {
		if (solicitar_turno(entrenador_listo) == EXIT_FAILURE)
			break;
		quantum--;
		//pthread_mutex_lock(&mutex_planificador_turno);
	}

	if ( (!(entrenador_listo->bloqueado)) && (!(entrenador_listo->objetivo_cumplido)) ) {
		list_add(cola_de_listos, entrenador_listo);
		signalSemaforo(semaforo_de_listos);
	}

	return EXIT_SUCCESS;
}

int correr_srdf() {
	return EXIT_SUCCESS;
}

t_sesion_entrenador * buscar_entrenador_por_simbolo(char symbol_expected) {
	t_sesion_entrenador * entrenador = NULL;

	bool find_trainer(t_sesion_entrenador * s_t) {
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

t_sesion_entrenador * buscar_entrenador_por_socket(int expected_fd) {
	t_sesion_entrenador * entrenador = NULL;

	bool find_trainer2(t_sesion_entrenador * s_t) {
		return (s_t->socket == expected_fd);
	}
	entrenador = list_find(entrenadores_conectados, (void *) find_trainer2);

	return entrenador;
}

//**********************************************************************************************

int enviar_ubicacion_pokenest(int socket, int id_pokenest) {

	int _on_error() {
		t_posicion * pos_error = malloc(sizeof(t_posicion));
		pos_error->x = -1;
		pos_error->y = -1;
		void * coord_error = malloc(sizeof(t_posicion));
		memcpy(coord_error, &(pos_error->x), 4);
		memcpy(coord_error + 4, &(pos_error->y), 4);

		send(socket, coord_error, sizeof(t_posicion), 0);

		free(pos_error);
		free(coord_error);

		return EXIT_FAILURE;
	}

	t_pokenest * pokenest = buscar_pokenest_por_id(id_pokenest);

	if (pokenest == NULL)
		return _on_error();

	void * coordenadas = malloc(sizeof(t_posicion));
	memcpy(coordenadas, &(pokenest->posicion->x), 4);
	memcpy(coordenadas + 4, &(pokenest->posicion->y), 4);

	if (send(socket, coordenadas, sizeof(t_posicion), 0) < 0)
		return _on_error();

	free(coordenadas);

	t_sesion_entrenador * entrenador = buscar_entrenador_por_socket(socket);

	if (entrenador == NULL)
		return EXIT_FAILURE;

	entrenador->posicionObjetivo->x = pokenest->posicion->x;
	entrenador->posicionObjetivo->y = pokenest->posicion->y;

	pthread_mutex_unlock(&mutex_planificador_turno);


	return EXIT_SUCCESS;
}

int avanzar_posicion_entrenador(int socket, int buffer_size) {

	void * buffer_in = malloc(buffer_size);
	t_posicion * movimiento = malloc(sizeof(t_posicion));

	if ( recv(socket, buffer_in, buffer_size, 0) < 0 )
		return EXIT_FAILURE;

	memcpy(&(movimiento->x), buffer_in, 4);
	memcpy(&(movimiento->y), buffer_in + 4, 4);

	t_sesion_entrenador * entrenador = buscar_entrenador_por_socket(socket);

	if (entrenador == NULL)
		return EXIT_FAILURE;

	entrenador->posicion->x = movimiento->x;
	entrenador->posicion->y = movimiento->y;

	//actualizar interfaz grafica

	free(buffer_in);
	free(movimiento);

	pthread_mutex_unlock(&mutex_planificador_turno);

	return EXIT_SUCCESS;
}

int atrapar_pokemon(int socket) {

	//se fija si el entrenador esta realmente en un pokenest
	t_sesion_entrenador * entrenador = buscar_entrenador_por_socket(socket);

	if (entrenador == NULL)
		return EXIT_FAILURE;

	if (!(esta_en_pokenest(entrenador)))
		return EXIT_FAILURE;

	//ok: se fija el pokemon de ese pokenest en el fd
	t_pokenest * pokenest = buscar_pokenest_por_ubicacion(entrenador->posicionObjetivo->x, entrenador->posicionObjetivo->y);

	if (pokenest == NULL)
		return EXIT_FAILURE;

	int stock_pokemons = list_size(pokenest->pokemones);

	if ( stock_pokemons > 0) {
		//si hay stock le envio la ruta del pokemon
		t_pokemon * pokemon_capturado = list_get(pokenest->pokemones, 0);
		list_remove(pokenest->pokemones, 0);
		list_add(entrenador->pokemonesCapturados, (void *) pokemon_capturado);

		char * ruta_pkm = string_new();

		string_append(&(ruta_pkm), "Mapas/");
		string_append(&(ruta_pkm), nombreMapa);
		string_append(&(ruta_pkm), "/PokeNests/");
		string_append(&(ruta_pkm), pokenest->nombre);
		string_append(&(ruta_pkm), "/");
		string_append(&(ruta_pkm), pokemon_capturado->nombreArchivo);

		int ruta_length = string_length(ruta_pkm);

		enviar_header(_CAPTURAR_PKM, ruta_length, socket);

		if ( send(socket, ruta_pkm, ruta_length, 0) < 0 )
			return EXIT_FAILURE;

		t_header * header = recibir_header(socket);

		if (header == NULL)
			return EXIT_FAILURE;

		switch (header->identificador) {
			case _QUEDAN_OBJETIVOS:
				//NO HAGO NADA EN ESPECIAL
				entrenador->objetivo_cumplido = false;
				break;
			case _OBJETIVO_CUMPLIDO:
				procesar_objetivo_cumplido(entrenador);
				break;
			default:
				break;
		}

	} else {
		//si no hay stock lo mando a bloqueados de ese pokenest
		entrenador->bloqueado = true;
		time(&(entrenador->momentoBloqueado));
		queue_push(pokenest->entrenadoresBloqueados, (void *) entrenador);
	}

	//desbloqueo el planificador
	pthread_mutex_unlock(&mutex_planificador_turno);

	return EXIT_SUCCESS;
}

bool esta_en_pokenest(t_sesion_entrenador * entrenador) {

	return (bool) ((entrenador->posicion->x == entrenador->posicionObjetivo->x)
			&& (entrenador->posicion->y == entrenador->posicionObjetivo->y));
}

int procesar_objetivo_cumplido(t_sesion_entrenador * entrenador) {
	//le envio la ruta de la medalla
	//calculo los tiempos y se los envio

	int datos_size = 0;

	entrenador->objetivo_cumplido = true;

	char * ruta_medalla = string_new();
	string_append(&(ruta_medalla), ruta_directorio);
	string_append(&(ruta_medalla), "Mapas/");
	string_append(&(ruta_medalla), nombreMapa);
	string_append(&(ruta_medalla), "/");
	string_append(&(ruta_medalla), metadata->medallaArchivo);

	datos_size += string_length(ruta_medalla);

	void * datos = malloc(datos_size);
	//datos:
	//ruta de la medalla
	//tiempo bloqueado
	//cuantos dl
	//tiempo total en el mapa

	//!!

	enviar_header(_DATOS_FINALES, datos_size, entrenador->socket);
	send(entrenador->socket, datos, datos_size, 0);

	return EXIT_SUCCESS;
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
