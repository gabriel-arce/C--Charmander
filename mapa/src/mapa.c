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

	if (conf_file == NULL) {
		pthread_mutex_lock(&mutex_log);
		log_trace(logger, "[ERROR]: Config NULL en leer_metadata_mapa.");
		pthread_mutex_unlock(&mutex_log);
		exit(EXIT_FAILURE);
	}

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
			if (cola_de_prioridad_SRDF != NULL)
				cola_de_prioridad_SRDF = list_create();
		} else {
			log_trace(logger, "[ERROR]: Algoritmo no reconocido.");
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
	logger = log_create(LOG_FILE, "MAPA log file", false, LOG_LEVEL_TRACE);
	log_info(logger, "MAPA %s iniciado.", nombreMapa);
}

void cargar_pokenests() {
	char * dir_pokenests = string_new();
	string_append(&dir_pokenests, ruta_directorio);
	string_append_with_format(&dir_pokenests, "Mapas/%s/PokeNests", nombreMapa);

	DIR * d = opendir(dir_pokenests);

	if (!d) {
		pthread_mutex_lock(&mutex_log);
		log_trace(logger, "No se pudo abrir el directorio: [ %s ]", dir_pokenests);
		pthread_mutex_unlock(&mutex_log);
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
//				int path_length;
//				char path[PATH_MAX];
//
//				path_length = snprintf(path, PATH_MAX, "%s/%s", dir_pokenests,
//						d_name);
//				if (path_length >= PATH_MAX) {
//					pthread_mutex_lock(&mutex_log);
//					log_trace(logger, "[ERROR]: Ruta demasiado larga.");
//					pthread_mutex_unlock(&mutex_log);
//					exit(EXIT_FAILURE);
//				}
				char * path = string_new();
				string_append_with_format(&path, "%s/%s", dir_pokenests, d_name);

				//now create the logical pokenest
				t_pokenest * pknst = malloc(sizeof(t_pokenest));
				pknst->nombre = string_duplicate(d_name);
				pknst->pokemones = list_create();
				pknst->posicion = malloc(sizeof(t_posicion));

				//read its files
				DIR * d_pknst = opendir(path);

				if (d_pknst == NULL) {
					pthread_mutex_lock(&mutex_log);
					log_trace(logger, "[ERROR]: No se pudo abrir el directorio: %s", path);
					pthread_mutex_unlock(&mutex_log);
					exit(EXIT_FAILURE);
				}

				while(1) {
					struct dirent * f_pknst = readdir(d_pknst);

					if (!f_pknst)
						break;

					if ( (f_pknst->d_type & DT_DIR ) || (strcmp(f_pknst->d_name, "..") == 0) || (strcmp(f_pknst->d_name, ".") == 0) )
						continue;

//					char path_f_pknst[PATH_MAX];
//
//					snprintf(path_f_pknst, PATH_MAX, "%s/%s", path, f_pknst->d_name);

					char * path_f_pknst = string_new();
					string_append_with_format(&path_f_pknst, "%s/%s", path, f_pknst->d_name);

					// if its the metadata go on
					if ( string_equals_ignore_case(f_pknst->d_name, "metadata") ) {

						t_config * m_pknst = config_create(path_f_pknst);

						if (m_pknst == NULL) {
							pthread_mutex_lock(&mutex_log);
							log_trace(logger, "[ERROR]: Config NULL en cargar_pokenests() en la ruta %s", path_f_pknst);
							pthread_mutex_unlock(&mutex_log);
							exit(EXIT_FAILURE);
						}

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

					if (dat_pkm == NULL) {
						pthread_mutex_lock(&mutex_log);
						log_trace(logger, "[ERROR]: Config NULL en cargar_pokenests()");
						pthread_mutex_unlock(&mutex_log);
						exit(EXIT_FAILURE);
					}

					t_pkm * pkm = malloc(sizeof(t_pkm));
					pkm->nombre = string_duplicate(d_name);
					pkm->nombreArchivo = string_duplicate(f_pknst->d_name);
					pkm->nivel = getIntProperty(dat_pkm, "Nivel");
					pkm->capturado = false;
					pkm->id_pokenest = pknst->identificador;

					list_add(pknst->pokemones, pkm);

					free(dat_pkm);
					free(path_f_pknst);
				}

				// <<------ END OF PROCESSING A NEW POKENEST
				ordenar_pokemons(pknst->pokemones);
				list_add(lista_de_pokenests, pknst);
				//Interfaz grafica
//				CrearCaja(items_mapa, pknst->identificador, pknst->posicion->x,
//						pknst->posicion->y, list_size(pknst->pokemones));

				free(path);
			}
		}
	}

	/* Release everything. */
	if (closedir(d)) {
		pthread_mutex_lock(&mutex_log);
		log_trace(logger, "[ERROR]: No se pudo cerrar el directorio: [ %s ]", dir_pokenests);
		pthread_mutex_unlock(&mutex_log);
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

	/*"address already in use" error message */
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
		log_trace(logger, "[ERROR]: Server-setsockopt() error");
		exit(1);
	}

	/* bind */
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(metadata->ip);
	serveraddr.sin_port = htons(metadata->puerto);

	memset(&(serveraddr.sin_zero), '\0', 8);

	if (bind(listener, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {
		log_trace(logger, "[ERROR]: Server-bind() error");
		exit(1);
	}

	/* listen */
	if (listen(listener, 10) == -1) {
		log_trace(logger, "[ERROR]: Server-listen() error");
		exit(1);
	}

	/* add the listener to the master set */
	FD_SET(listener, &master_fdset);

	/* keep track of the biggest file descriptor */
	fdmax = listener;

	/* loop for new entries */
	while(1) {

		read_fds = master_fdset;

		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			log_trace(logger, "[ERROR]: Server-select() error");
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
						pthread_mutex_lock(&mutex_log);
						log_trace(logger, "[ERROR]: Server-accept() error");
						pthread_mutex_unlock(&mutex_log);
					} else {

						pthread_mutex_lock(&mutex_servidor);

						FD_SET(newfd, &master_fdset); /* add to master set */

						if (newfd > fdmax)
							fdmax = newfd;

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

	pthread_mutex_lock(&mutex_log);
	log_trace(logger, "Nueva conexion desde el socket %d, entrenador %c",
			nuevo_entrenador->socket, nuevo_entrenador->simbolo_entrenador);
	pthread_mutex_unlock(&mutex_log);

	if (nuevo_entrenador == NULL)
		return -1;

//	Interfaz grafica
//	pthread_mutex_lock(&mutex_gui);
//	nivel_gui_dibujar(items_mapa, nombreMapa);
//	pthread_mutex_unlock(&mutex_gui);

	list_add(entrenadores_conectados, nuevo_entrenador);

	//manda a listos al entrenador
	if (agregar_a_cola(nuevo_entrenador, cola_de_listos, mutex_cola_listos) != -1)
		signalSemaforo(semaforo_de_listos);

	return 0;
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
	pthread_mutex_init(&(trainer_sesion->mutex_entrenador), 0);

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
	t_entrenador * entrenador_listo = pop_entrenador();

	if (entrenador_listo == NULL)
		return EXIT_FAILURE;

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
		if (agregar_a_cola(entrenador_listo, cola_de_listos, mutex_cola_listos) != -1)
			signalSemaforo(semaforo_de_listos);
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
		if (agregar_a_cola(entrenador, cola_de_listos, mutex_cola_listos) != -1)
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

	pthread_mutex_lock(&mutex_servidor);

	int buffer_size = sizeof(t_header);
	void * buffer_in = malloc(buffer_size);

	int nbytes_recv = recv(entrenador->socket, buffer_in, buffer_size, 0);

	t_header * header = deserializar_header(buffer_in);

	//DESCONEXION DE UN ENTRENADOR
	if (nbytes_recv <= 0) {
		desconexion_entrenador(entrenador, nbytes_recv);
		free(header);
		pthread_mutex_unlock(&mutex_servidor);
		return EXIT_FAILURE;
	}

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

	pthread_mutex_unlock(&mutex_servidor);

	return EXIT_SUCCESS;
}

int desconexion_entrenador(t_entrenador * entrenador, int nbytes_recv) {
	if (nbytes_recv == 0) {
		pthread_mutex_lock(&mutex_log);
		log_trace(logger, "Se desconecto el entrenador %c en el socket %d",
				entrenador->simbolo_entrenador, entrenador->socket);
		pthread_mutex_unlock(&mutex_log);
	}

	if (nbytes_recv < 0) {
		pthread_mutex_lock(&mutex_log);
		log_trace(logger, "[ERROR]: Error en el recv desde el socket %d", entrenador->socket);
		pthread_mutex_unlock(&mutex_log);
	}

	if (nbytes_recv == 1) {
		pthread_mutex_lock(&mutex_log);
		log_trace(logger, "[DEADLOCK]: Se desconecto el entrenador %c por haber perdido la batalla");
		pthread_mutex_unlock(&mutex_log);
	}

	FD_CLR(entrenador->socket, &master_fdset);
	shutdown(entrenador->socket, 2);
	close(entrenador->socket);
	entrenador->socket = -1;

	//lo saco de listos (si esta en la cola)
	sacar_de_listos(entrenador);

	//lo saco de la lista de entrenadores conectados
	sacar_de_conectados(entrenador);

	//fijarse a nivel pokenest las colas de bloqueados
	pthread_mutex_lock(&mutex_cola_bloqueados);
	sacar_de_bloqueados(entrenador);
	pthread_mutex_unlock(&mutex_cola_bloqueados);

	//liberar pokemons capturados
	liberar_pokemons(entrenador);

	//lo saco de la lista de elementos de la gui y la actualizo
	//Interfaz grafica
//	pthread_mutex_lock(&mutex_gui);
//	BorrarItem(items_mapa, entrenador->simbolo_entrenador);
//	nivel_gui_dibujar(items_mapa, nombreMapa);
//	pthread_mutex_unlock(&mutex_gui);

	//entrenador_destroyer(entrenador);
	entrenador->simbolo_entrenador = ' ';
	free(entrenador->nombre_entrenador);
	free(entrenador->posicion);
	free(entrenador->posicionObjetivo);

	return EXIT_FAILURE;
}

int enviar_ubicacion_pokenest(t_entrenador * entrenador, int id_pokenest) {

	printf("Enviar ubicacion, entrenador:	%c\n", entrenador->simbolo_entrenador);

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

		pthread_mutex_unlock(&(entrenador->mutex_entrenador));

		return EXIT_FAILURE;
	}

	pthread_mutex_lock(&(entrenador->mutex_entrenador));

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

	pthread_mutex_unlock(&(entrenador->mutex_entrenador));

	return EXIT_SUCCESS;
}

int avanzar_posicion_entrenador(t_entrenador * entrenador, int buffer_size) {

	printf("Avanzar:	%c\n", entrenador->simbolo_entrenador);

	int _on_error() {
		keep_running = false;
		quantum_actual = 0;
		enviar_header(_RESULTADO_OPERACION, 0, entrenador->socket);
		pthread_mutex_unlock(&(entrenador->mutex_entrenador));
		return EXIT_FAILURE;
	}

	pthread_mutex_lock(&(entrenador->mutex_entrenador));

	void * buffer_in = malloc(buffer_size);
	t_posicion * movimiento = malloc(sizeof(t_posicion));

	if ( recv(entrenador->socket, buffer_in, buffer_size, 0) < 0 )
		return _on_error();

	memcpy(&(movimiento->x), buffer_in, 4);
	memcpy(&(movimiento->y), buffer_in + 4, 4);

	usleep(metadata->planificador->retardo_turno);

	entrenador->posicion->x = movimiento->x;
	entrenador->posicion->y = movimiento->y;

//	Interfaz grafica
//	pthread_mutex_lock(&mutex_gui);
//	MoverPersonaje(items_mapa, entrenador->simbolo_entrenador, movimiento->x, movimiento->y);
//	nivel_gui_dibujar(items_mapa, nombreMapa);
//	pthread_mutex_unlock(&mutex_gui);

	free(buffer_in);
	free(movimiento);

	enviar_header(_RESULTADO_OPERACION, 1, entrenador->socket);
	pthread_mutex_unlock(&(entrenador->mutex_entrenador));

	return EXIT_SUCCESS;
}

int atrapar_pokemon(t_entrenador * entrenador) {

	int _on_error() {
		//TODO handlear el error de forma copada
		keep_running = false;
		return EXIT_FAILURE;
	}

	if (!(esta_en_pokenest(entrenador)))
		return _on_error();

	t_pokenest * pokenest = buscar_pokenest_por_ubicacion(entrenador->posicionObjetivo->x, entrenador->posicionObjetivo->y);

	if (pokenest == NULL)
		return _on_error();

	usleep(metadata->planificador->retardo_turno);

	quantum_actual = 0;
	keep_running = false;

	entrenador->bloqueado = true;

	t_bloqueado * entrenador_bloqueado = malloc(sizeof(t_bloqueado));
	entrenador_bloqueado->entrenador = entrenador;
	entrenador_bloqueado->pokenest = pokenest;

	printf("Atrapar (%c, %c)\n", entrenador->simbolo_entrenador, pokenest->identificador);

	time(&(entrenador->momentoBloqueado));

	pthread_mutex_lock(&mutex_cola_bloqueados);
	list_add(cola_de_bloqueados, entrenador_bloqueado);
	pthread_mutex_unlock(&mutex_cola_bloqueados);
	signalSemaforo(semaforo_de_bloqueados);

	return EXIT_SUCCESS;
}

int procesar_objetivo_cumplido(t_entrenador * entrenador) {

//	pthread_mutex_lock(&(entrenador->mutex_entrenador));

	pthread_mutex_lock(&(mutex_log));
	log_trace(logger, "El entrenador %c ha logrado su objetivo.", entrenador->simbolo_entrenador);
	pthread_mutex_unlock(&(mutex_log));

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
	int offset = 0;
	memset(datos, 0, datos_size);
	memcpy(datos, &(tiempo_tot_mapa), sizeof(double));
	offset += sizeof(double);
	memcpy(datos + offset, &(entrenador->tiempoBloqueado), sizeof(double));
	offset += sizeof(double);
	memcpy(datos + offset, &(entrenador->deadlocksInvolucrados), sizeof(int));

	enviar_header(_DATOS_FINALES, datos_size, entrenador->socket);
	send(entrenador->socket, datos, datos_size, 0);

	desconexion_entrenador(entrenador, 0);

//	pthread_mutex_lock(&(entrenador->mutex_entrenador));

	free(datos);

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

void atender_bloqueados() {

	int cantidad_bloqueados = 0;
	int i;

	while (true) {
		waitSemaforo(semaforo_de_bloqueados);

		//TODO GUARDA CON ESTO!!!!
		pthread_mutex_lock(&mutex_cola_bloqueados);

		cantidad_bloqueados = list_size(cola_de_bloqueados);

		for (i = 0; i < cantidad_bloqueados; i++) {
			t_bloqueado * b = list_get(cola_de_bloqueados, i);

			if (b == NULL)
				continue;

			t_pkm * pkm = obtener_primer_no_capturado(b->pokenest);

			if (pkm != NULL /*pudo capturar*/) {
				list_remove(cola_de_bloqueados, i);
				i--;
				generar_captura(b->entrenador, b->pokenest, pkm);
			}
		}

		pthread_mutex_unlock(&mutex_cola_bloqueados);
	}

}

int generar_captura(t_entrenador * entrenador, t_pokenest * pokenest, t_pkm * pokemon) {

	int _on_error() {
		pokemon->capturado = false;
		pthread_mutex_lock(&(entrenador->mutex_entrenador));
		entrenador->conoce_ubicacion = true;
		pokemon_remover(pokemon, entrenador->pokemonesCapturados);
		pthread_mutex_unlock(&(entrenador->mutex_entrenador));
		//interfaz grafica
		//incrementar_recurso(pokenest->identificador);
		return -1;
	}

	time_t tiempo_desbloqueo;
	time(&tiempo_desbloqueo);

	pokemon->capturado = true;
	pthread_mutex_lock(&(entrenador->mutex_entrenador));
	list_add(entrenador->pokemonesCapturados, pokemon);
	entrenador->conoce_ubicacion = false;
	entrenador->bloqueado = false;
	entrenador->tiempoBloqueado += difftime(tiempo_desbloqueo, entrenador->momentoBloqueado);
	entrenador->momentoBloqueado = 0;
	pthread_mutex_unlock(&(entrenador->mutex_entrenador));

	char * ruta_pkm = string_duplicate(ruta_directorio);
	string_append_with_format(&ruta_pkm, "Mapas/%s/PokeNests/%s/%s", nombreMapa,
			pokenest->nombre, pokemon->nombreArchivo);

	if (enviar_ruta_pkm(ruta_pkm, entrenador->socket) == -1)
		return _on_error();

//	Interfaz grafica
	//pthread_mutex_lock(&mutex_gui);
	//restarRecurso(items_mapa, pokenest->identificador);
	//nivel_gui_dibujar(items_mapa, nombreMapa);
	//pthread_mutex_unlock(&mutex_gui);

	t_header * header = recibir_header(entrenador->socket);

	if (header == NULL)
		return _on_error();

	switch (header->identificador) {
		case _QUEDAN_OBJETIVOS:
			entrenador->objetivo_cumplido = false;
			if (agregar_a_cola(entrenador, cola_de_listos, mutex_cola_listos) != -1)
				signalSemaforo(semaforo_de_listos);
			break;
		case _OBJETIVO_CUMPLIDO:
			// TODO deberia crear un hilo dettach para que pueda seguir con los demas bloqueados
			procesar_objetivo_cumplido(entrenador);
			break;
		default:
			break;
	}

	return EXIT_SUCCESS;
}
