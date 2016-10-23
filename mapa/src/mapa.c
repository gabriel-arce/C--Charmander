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
	string_append_with_format(&ruta, "Mapas/%s/metadata", nombreMapa);

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

	cargar_medalla();

	free(conf_file);
	free(ruta);
}

void cargar_medalla() {
	char * ruta_medalla = string_duplicate(ruta_directorio);
	string_append_with_format(&ruta_medalla, "%s/medalla-%s.jpg", nombreMapa, nombreMapa);

	metadata->medallaArchivo = string_duplicate(ruta_medalla);

	free(ruta_medalla);
}

void crear_archivo_log() {
	logger = log_create(LOG_FILE, "MAPA log file", true, LOG_LEVEL_TRACE);
	log_info(logger, "MAPA %s iniciado.", nombreMapa);
}

void cargar_pokenests() {
	char * dir_pokenests = string_new();
	string_append(&dir_pokenests, ruta_directorio);
	string_append_with_format(&dir_pokenests, "Mapas/%s/PokeNests", nombreMapa);

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
					pkm->capturado = false;
					pkm->id_pokenest = pknst->identificador;

					list_add(pknst->pokemones, pkm);

					free(dat_pkm);
				}

				// <<------ END OF PROCESSING A NEW POKENEST
				ordenar_pokemons(pknst->pokemones);
				list_add(lista_de_pokenests, pknst);
				//Interfaz grafica
//				CrearCaja(items_mapa, pknst->identificador, pknst->posicion->x,
//						pknst->posicion->y, list_size(pknst->pokemones));
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
//	fd_set master_fdset;
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
//	printf("Server-socket() is OK...\n");

	/*"address already in use" error message */
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
		perror("Server-setsockopt() error");
		exit(1);
	}
	//printf("Server-setsockopt() is OK...\n");

	/* bind */
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(metadata->ip);
	serveraddr.sin_port = htons(metadata->puerto);

	memset(&(serveraddr.sin_zero), '\0', 8);

	if (bind(listener, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {
		perror("Server-bind() error");
		exit(1);
	}
	//printf("Server-bind() is OK...\n");

	/* listen */
	if (listen(listener, 10) == -1) {
		perror("Server-listen() error");
		exit(1);
	}
	//printf("Server-listen() is OK...\n");

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

						pthread_mutex_lock(&mutex_servidor);

//						printf("Server-accept() is OK...\n");

						FD_SET(newfd, &master_fdset); /* add to master set */

						if (newfd > fdmax)
							fdmax = newfd;

//						printf("New connection from %s on socket %d\n",
//								inet_ntoa(clientaddr.sin_addr), newfd);

						t_header * handshake = recibir_header(newfd);

						if (handshake->identificador == _ID_HANDSHAKE) {
							procesar_nuevo_entrenador(newfd, handshake->tamanio);
						} else {
							close(newfd);
							FD_CLR(newfd, &read_fds);
						}

						pthread_mutex_unlock(&mutex_servidor);

					}

				}
			}
		}
	}
}

int procesar_nuevo_entrenador(int socket_entrenador, int buffer_size) {
	t_entrenador * nuevo_entrenador = recibir_datos_entrenador(socket_entrenador, buffer_size);

	if (nuevo_entrenador == NULL)
		return -1;

//	printf("\nSe conecto %s en el socket %d con el simbolo %c\n",
//			nuevo_entrenador->nombre_entrenador, nuevo_entrenador->socket,
//			nuevo_entrenador->simbolo_entrenador);

//	Interfaz grafica
//	pthread_mutex_lock(&mutex_gui);
//	nivel_gui_dibujar(items_mapa, nombreMapa);
//	pthread_mutex_unlock(&mutex_gui);

	list_add(entrenadores_conectados, nuevo_entrenador);

	//manda a listos al entrenador
	//agregarEntrenadorAListos(nuevo_entrenador);
	list_add(cola_de_listos, nuevo_entrenador);
	signalSemaforo(semaforo_de_listos);

	return 0;
}

void agregarEntrenadorAListos(t_entrenador * nuevo_entrenador){

	switch (metadata->planificador->algth) {
	case RR:
		list_add(cola_de_listos, nuevo_entrenador);
		break;
	case SRDF:
		list_add(cola_de_prioridad_SRDF, nuevo_entrenador);
		break;
	}

	signalSemaforo(semaforo_de_listos);

}

t_entrenador * recibir_datos_entrenador(int socket_entrenador, int data_buffer_size) {

	void * data_buffer = malloc(data_buffer_size);
	if (recv(socket_entrenador, data_buffer, data_buffer_size, 0) <= 0)
		return NULL;

	t_entrenador * trainer_sesion = malloc(sizeof(t_entrenador));

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
	trainer_sesion->conoce_ubicacion = false;

	//Interfaz grafica
//	pthread_mutex_lock(&mutex_gui);
//	CrearPersonaje(items_mapa, trainer_sesion->simbolo_entrenador,
//			trainer_sesion->posicion->x, trainer_sesion->posicion->y);
//
//	nivel_gui_dibujar(items_mapa, nombreMapa);
//	pthread_mutex_unlock(&mutex_gui);

	return trainer_sesion;
}

//*******************************************************************************************

void run_scheduler_thread() {

	while (true) {
		waitSemaforo(semaforo_de_listos);
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

int correr_rr() {

	quantum_actual = metadata->planificador->quantum;
	keep_running = true;
	int result;

	// Selecciono a un entrenador en la cola de listos
	t_entrenador * entrenador_listo = list_get(cola_de_listos, 0);

	if (entrenador_listo == NULL)
		return EXIT_FAILURE;

	list_remove(cola_de_listos, 0);
	entrenador_corriendo = entrenador_listo;

	// Ejecuto al entrenador listo
	while ( keep_running ) {

		pthread_mutex_lock(&mutex_planificador_turno);

		if ( (quantum_actual <= 0) || (entrenador_listo->bloqueado) || (entrenador_listo->objetivo_cumplido) ) {
			keep_running = false;
			quantum_actual = 0;
			entrenador_corriendo = NULL;
			pthread_mutex_unlock(&mutex_planificador_turno);
			break;
		}

		result = trainer_handler(entrenador_listo);

		if (result == EXIT_FAILURE) {
			keep_running = false;
			quantum_actual = 0;
			entrenador_corriendo = NULL;
			pthread_mutex_unlock(&mutex_planificador_turno);
			break;
		}

		quantum_actual--;

		pthread_mutex_unlock(&mutex_planificador_turno);

	}

	// Verifico si el entrenador vuelve a la cola de listos
	if ( (!(entrenador_listo->bloqueado)) && (!(entrenador_listo->objetivo_cumplido)) && (result != EXIT_FAILURE) ) {
		list_add(cola_de_listos, entrenador_listo);
		signalSemaforo(semaforo_de_listos);
//		agregarEntrenadorAListos(entrenador_listo);
	}

	entrenador_corriendo = NULL;

	return EXIT_SUCCESS;
}

int correr_srdf() {
	int cantidad_en_listos = list_size(cola_de_listos);
	int i;
	int result = EXIT_SUCCESS;
	keep_running = true;

	void una_operacion(t_entrenador * ent) {
		result = trainer_handler(ent);
		enviar_header(_RESULTADO_OPERACION, result, ent->socket);

		if (result == EXIT_FAILURE) {
			keep_running = false;
		}
	}

	// Paso 1: Atiedo una sola operacion de a aquellos que no
	// conozcan su distancia a la proxima pokenest
	for (i = 0; i < cantidad_en_listos; i++) {
		t_entrenador * e = list_get(cola_de_listos, i);

		if ( !(e->conoce_ubicacion) ) {
			una_operacion(e);
		}
	}

	// Paso 2: Si ya no hay entrenador sin conocer su proxima ubicacion
	// ordeno la cola de listos segun el calculo de distancias y saco el primero
	t_entrenador * entrenador = calcularSRDF();
	entrenador_corriendo = entrenador;

	while (keep_running) {
		if ( (entrenador->bloqueado) || (entrenador->objetivo_cumplido) ) {
			keep_running = false;
			break;
		} else {
			una_operacion(entrenador);
		}
	}

	entrenador_corriendo = NULL;

	// Verifico si el entrenador vuelve a la cola de listos
	if ( (!(entrenador->bloqueado)) && (!(entrenador->objetivo_cumplido)) && (result != EXIT_FAILURE) ) {
//		agregarEntrenadorAListos(entrenador);
		list_add(cola_de_listos, entrenador);
		signalSemaforo(semaforo_de_listos);
	}

	return result;
}

t_entrenador * calcularSRDF(){

	t_entrenador * entrenadorConMenorDistancia;

	list_sort(cola_de_listos, (void*) entrenadorMasCercaDePokenest);

	entrenadorConMenorDistancia = list_get(cola_de_listos,0);

	return entrenadorConMenorDistancia;

}

bool entrenadorMasCercaDePokenest(t_entrenador * entrenador_cerca, t_entrenador * entrenador_mas_cerca){
	return ( calcularDistanciaAPokenest(entrenador_cerca) < calcularDistanciaAPokenest(entrenador_mas_cerca));

}

int calcularDistanciaAPokenest(t_entrenador* entrenador){

	int movimientosRestantesEnX = abs(entrenador->posicionObjetivo->x - entrenador->posicion->x);
	int movimientosRestantesEnY = abs(entrenador->posicionObjetivo->y - entrenador->posicion->y);

	return (movimientosRestantesEnX + movimientosRestantesEnY);
}

int trainer_handler(t_entrenador * entrenador) {

	int buffer_size = sizeof(t_header);
	void * buffer_in = malloc(buffer_size);

	int nbytes_recv = recv(entrenador->socket, buffer_in, buffer_size, 0);

	t_header * header = deserializar_header(buffer_in);

	//DESCONEXION DE UN ENTRENADOR
	if (nbytes_recv <= 0)
		return desconexion_entrenador(entrenador, nbytes_recv);

	//OPERACIONES ENTRENADOR
	switch (header->identificador) {
		case _UBICACION_POKENEST:
			enviar_ubicacion_pokenest(entrenador, header->tamanio);
			break;
		case _MOVER_XY:
			avanzar_posicion_entrenador(entrenador, header->tamanio);
			break;
		case _CAPTURAR_PKM:
			atrapar_pokemon(entrenador);
			break;
		default:
			break;
	}

	free(header);

	return EXIT_SUCCESS;
}

int desconexion_entrenador(t_entrenador * entrenador, int nbytes_recv) {
	if (nbytes_recv == 0)
		log_trace(logger, "Se desconecto el entrenador %c en el socket %d", entrenador->simbolo_entrenador, entrenador->socket);


	if (nbytes_recv < 0)
		log_trace(logger, "Error en el recv desde el socket %d", entrenador->socket);

	FD_CLR(entrenador->socket, &master_fdset);
	close(entrenador->socket);

	//lo saco de listos (si esta en la cola)
	sacar_de_listos(entrenador);

	//lo saco de la lista de entrenadores conectados
	sacar_de_conectados(entrenador);

	//fijarse a nivel pokenest las colas de bloqueados
	sacar_de_bloqueados(entrenador);

	//liberar pokemons capturados
	liberar_pokemons(entrenador);

	//lo saco de la lista de elementos de la gui y la actualizo
	//Interfaz grafica
//	pthread_mutex_lock(&mutex_gui);
//	BorrarItem(items_mapa, entrenador->simbolo_entrenador);
//	nivel_gui_dibujar(items_mapa, nombreMapa);
//	pthread_mutex_unlock(&mutex_gui);

	entrenador_destroyer(entrenador);

	return EXIT_FAILURE;
}

int enviar_ubicacion_pokenest(t_entrenador * entrenador, int id_pokenest) {

//	printf("%s %d >> Ubicacion Pokenest\n", entrenador->nombre_entrenador, entrenador->socket);

	int _on_error() {
		t_posicion * pos_error = malloc(sizeof(t_posicion));
		pos_error->x = -1;
		pos_error->y = -1;
		void * coord_error = malloc(sizeof(t_posicion));
		memcpy(coord_error, &(pos_error->x), 4);
		memcpy(coord_error + 4, &(pos_error->y), 4);

		send(entrenador->socket, coord_error, sizeof(t_posicion), 0);

		free(pos_error);
		free(coord_error);

		keep_running = false;
		quantum_actual = 0;

		return EXIT_FAILURE;
	}

	t_pokenest * pokenest = buscar_pokenest_por_id(id_pokenest);

	if (pokenest == NULL)
		return _on_error();

	void * coordenadas = malloc(sizeof(t_posicion));
	memcpy(coordenadas, &(pokenest->posicion->x), 4);
	memcpy(coordenadas + 4, &(pokenest->posicion->y), 4);

	usleep(metadata->planificador->retardo_turno);

	entrenador->posicionObjetivo->x = pokenest->posicion->x;
	entrenador->posicionObjetivo->y = pokenest->posicion->y;

	entrenador->conoce_ubicacion = true;

	if (send(entrenador->socket, coordenadas, sizeof(t_posicion), 0) < 0)
		return _on_error();

	free(coordenadas);

	return EXIT_SUCCESS;
}

int avanzar_posicion_entrenador(t_entrenador * entrenador, int buffer_size) {

//	printf("%s %d >> Avanzar Posicion\n", entrenador->nombre_entrenador, entrenador->socket);

	int _on_error() {
		keep_running = false;
		quantum_actual = 0;
		enviar_header(_RESULTADO_OPERACION, 0, entrenador->socket);
		return EXIT_FAILURE;
	}

	void * buffer_in = malloc(buffer_size);
	t_posicion * movimiento = malloc(sizeof(t_posicion));

	if ( recv(entrenador->socket, buffer_in, buffer_size, 0) < 0 )
		return _on_error();

	memcpy(&(movimiento->x), buffer_in, 4);
	memcpy(&(movimiento->y), buffer_in + 4, 4);

	usleep(metadata->planificador->retardo_turno);

	entrenador->posicion->x = movimiento->x;
	entrenador->posicion->y = movimiento->y;

	//actualizar interfaz grafica
//	pthread_mutex_lock(&mutex_gui);
//	MoverPersonaje(items_mapa, entrenador->simbolo_entrenador, movimiento->x, movimiento->y);
//	nivel_gui_dibujar(items_mapa, nombreMapa);
//	pthread_mutex_unlock(&mutex_gui);

	free(buffer_in);
	free(movimiento);

	enviar_header(_RESULTADO_OPERACION, 1, entrenador->socket);

	return EXIT_SUCCESS;
}

int atrapar_pokemon(t_entrenador * entrenador) {

//	printf("%s %d >> Atrapar Pokemon\n", entrenador->nombre_entrenador, entrenador->socket);

	int _on_error() {
		keep_running = false;
		return EXIT_FAILURE;
	}

	//se fija si el entrenador esta realmente en un pokenest
	if (!(esta_en_pokenest(entrenador)))
		return _on_error();

	//ok: se fija el pokemon de ese pokenest en el fd
	t_pokenest * pokenest = buscar_pokenest_por_ubicacion(entrenador->posicionObjetivo->x, entrenador->posicionObjetivo->y);

	if (pokenest == NULL)
		return _on_error();

	usleep(metadata->planificador->retardo_turno);

	//obtengo el primer que no este capturado
	t_pokemon * pkm_capt = obtener_primer_no_capturado(pokenest);

	if ( pkm_capt != NULL ) {

		pkm_capt->capturado = true;
		list_add(entrenador->pokemonesCapturados, (void *) pkm_capt);

		//si hay stock le envio la ruta del pokemon
		char * ruta_pkm = string_duplicate(ruta_directorio);
		string_append_with_format(&ruta_pkm, "Mapas/%s/PokeNests/%s/%s",
				nombreMapa, pokenest->nombre, pkm_capt->nombreArchivo);

		int ruta_length = string_length(ruta_pkm);

		void * buff = malloc(ruta_length);
		memset(buff, 0, ruta_length);
		memcpy(buff, ruta_pkm, ruta_length);

		enviar_header(_CAPTURAR_PKM, ruta_length, entrenador->socket);

		if ( send(entrenador->socket, buff, ruta_length, 0) < 0 ) {
			pokemon_remover(pkm_capt, entrenador->pokemonesCapturados);
			list_add(pokenest->pokemones, pkm_capt);
			return _on_error();
		}

		free(ruta_pkm);
		free(buff);

		entrenador->conoce_ubicacion = false;

		//actualizo interfaz grafica
//		pthread_mutex_lock(&mutex_gui);
//		restarRecurso(items_mapa, pokenest->identificador);
//		nivel_gui_dibujar(items_mapa, nombreMapa);
//		pthread_mutex_unlock(&mutex_gui);

		//Se pone en espera a que le responda el entrenador
		t_header * header = recibir_header(entrenador->socket);

		if (header == NULL) {
			pokemon_remover(pkm_capt, entrenador->pokemonesCapturados);
			pkm_capt->capturado = false;
			//incrementar_recurso(pokenest->identificador);
			return _on_error();
		}

		switch (header->identificador) {
			case _QUEDAN_OBJETIVOS:
				//NO HAGO NADA EN ESPECIAL
				entrenador->objetivo_cumplido = false;
				break;
			case _OBJETIVO_CUMPLIDO:
//				printf("objetivo cumplido de %s\n", entrenador->nombre_entrenador);
				log_trace(logger, "El entrenador %c ha logrado su objetivo.", entrenador->simbolo_entrenador);
				procesar_objetivo_cumplido(entrenador);
				break;
			default:
				break;
		}

	} else {
		//si no hay stock lo mando a bloqueados de ese pokenest

//		printf("NO HAY MAS POKEMONS, EL ENTRENADOR %s se bloqueó\n", entrenador->nombre_entrenador);
		log_trace(logger,
				"El entrenador %c se quedo bloqueado en el pokenest %c",
				entrenador->simbolo_entrenador, pokenest->identificador);

		entrenador->bloqueado = true;
		time(&(entrenador->momentoBloqueado));
		queue_push(pokenest->entrenadoresBloqueados, (void *) entrenador);

	}

	quantum_actual = 0;
	keep_running = false;

	return EXIT_SUCCESS;
}

int procesar_objetivo_cumplido(t_entrenador * entrenador) {

	entrenador->objetivo_cumplido = true;

	//calculo los tiempos y se los envio
	time_t tiempo_actual;
	time(&(tiempo_actual));
	double tiempo_tot_mapa = difftime(tiempo_actual, entrenador->tiempoDeIngresoAlMapa); //[seg]

	int datos_size = 2*sizeof(double) + sizeof(int);

	//datos:
	//tiempo total en el mapa
	//tiempo bloqueado
	//cuantos dl
	void * datos = malloc(datos_size);
	memcpy(datos, &(tiempo_tot_mapa), sizeof(double));
	memcpy(datos + sizeof(double), &(entrenador->tiempoBloqueado), sizeof(double));
	memcpy(datos + 2*sizeof(double), &(entrenador->deadlocksInvolucrados), sizeof(int));

	enviar_header(_DATOS_FINALES, datos_size, entrenador->socket);
	send(entrenador->socket, datos, datos_size, 0);

	desconexion_entrenador(entrenador, 0);

	return EXIT_SUCCESS;
}

//**********************************************************************************

void signal_handler(int signal) {
	if (signal == SIGUSR2) {
		pthread_mutex_lock(&mutex_planificador_turno);
		keep_running = false;
		quantum_actual = 0;
		destruir_metadata();
		leer_metadata_mapa(ruta_directorio);
		pthread_mutex_unlock(&mutex_planificador_turno);
	}
}
