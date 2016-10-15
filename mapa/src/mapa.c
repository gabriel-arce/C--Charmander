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
	string_append(&ruta, "/metadata");

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
	string_append(&ruta_medalla, nombreMapa);
	string_append(&ruta_medalla, "/medalla-");
	string_append(&ruta_medalla, nombreMapa);
	string_append(&ruta_medalla, ".jpg");

	metadata->medallaArchivo = string_duplicate(ruta_medalla);

	free(ruta_medalla);
}

void crear_archivo_log() {
	logger = log_create(LOG_FILE, "MAPA log file", true, LOG_LEVEL_TRACE);
	log_info(logger, "MAPA iniciado.");
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
	printf("Server-socket() is OK...\n");

	/*"address already in use" error message */
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
		perror("Server-setsockopt() error");
		exit(1);
	}
	printf("Server-setsockopt() is OK...\n");

	/* bind */
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(metadata->ip);
	serveraddr.sin_port = htons(metadata->puerto);

	memset(&(serveraddr.sin_zero), '\0', 8);

	if (bind(listener, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {
		perror("Server-bind() error");
		exit(1);
	}
	printf("Server-bind() is OK...\n");

	/* listen */
	if (listen(listener, 10) == -1) {
		perror("Server-listen() error");
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

						//pthread_mutex_lock(&mutex_servidor);

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

						//pthread_mutex_unlock(&mutex_servidor);

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

	printf("\nSe conecto %s en el socket %d con el simbolo %c\n",
			nuevo_entrenador->nombre_entrenador, nuevo_entrenador->socket,
			nuevo_entrenador->simbolo_entrenador);

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

//int correr_srdf() {
//	int result;
//
//	if(list_size(cola_de_prioridad_SRDF)  > 0){
//
//		entrenador_corriendo = list_get(cola_de_prioridad_SRDF,0);
//		list_remove(cola_de_prioridad_SRDF,0);
//
//		//Entrenador pide ubicacion de pokenest
//		result = trainer_handler(entrenador_corriendo);   //¿hacer algo con result?
//		//Lo agrego a la cola de listos posta
//		list_add(cola_de_listos, entrenador_corriendo);
//	}
//
//	else{
//		entrenador_corriendo = calcularSRDF();
//
//		while(1){
//
//			if ((entrenador_corriendo->bloqueado) || (entrenador_corriendo->objetivo_cumplido) ){
//				break;
//			}
//			else{
//				result = trainer_handler(entrenador_corriendo);
//			}
//		}
//	}
//
//	entrenador_corriendo = NULL;
//
//	return EXIT_SUCCESS;
//}

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
		return desconexion_entrenador(entrenador->socket, nbytes_recv);

	//OPERACIONES ENTRENADOR
	switch (header->identificador) {
		case _UBICACION_POKENEST:
			puts("ubicacion pknst");
			enviar_ubicacion_pokenest(entrenador, header->tamanio);
			break;
		case _MOVER_XY:
			puts("mover");
			avanzar_posicion_entrenador(entrenador, header->tamanio);
			break;
		case _CAPTURAR_PKM:
			puts("capturar pkm");
			atrapar_pokemon(entrenador);
			break;
		default:
			break;
	}

	free(header);

	return EXIT_SUCCESS;
}

int desconexion_entrenador(int socket, int nbytes_recv) {
	if (nbytes_recv == 0)
		printf("Socket %d disconnected\n", socket);

	if (nbytes_recv < 0)
		printf("recv error\n");

	close(socket);
	FD_CLR(socket, &master_fdset);

	//lo saco de listos (si esta en la cola)
	int listos = list_size(cola_de_listos);
	int i;
	for (i = 0; i < listos; i++) {
		t_entrenador * e = list_get(cola_de_listos, i);

		if (e->socket == socket) {
			list_remove(cola_de_listos, i);
			break;
		}
	}

	//lo saco de la lista de entrenadores conectados
	int totales = list_size(entrenadores_conectados);
	for (i = 0; i < totales; i++) {
		t_entrenador * e = list_get(entrenadores_conectados, i);

		if (e->socket == socket) {

			if (e == entrenador_corriendo) {
				keep_running = false;
				entrenador_corriendo = NULL;
				quantum_actual = 0;
			}

			list_remove(entrenadores_conectados, i);
			break;
		}
	}

	//liberar pokemons capturados
	//fijarse a nivel pokenest las colas de bloqueados

	return EXIT_FAILURE;
}

int enviar_ubicacion_pokenest(t_entrenador * entrenador, int id_pokenest) {

	printf("%s %d >> Ubicacion Pokenest\n", entrenador->nombre_entrenador, entrenador->socket);

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

//	sleep(metadata->planificador->retardo_turno);

	entrenador->posicionObjetivo->x = pokenest->posicion->x;
	entrenador->posicionObjetivo->y = pokenest->posicion->y;

	entrenador->conoce_ubicacion = true;

	if (send(entrenador->socket, coordenadas, sizeof(t_posicion), 0) < 0)
		return _on_error();

	free(coordenadas);

	return EXIT_SUCCESS;
}

int avanzar_posicion_entrenador(t_entrenador * entrenador, int buffer_size) {

	printf("%s %d >> Avanzar Posicion\n", entrenador->nombre_entrenador, entrenador->socket);

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

//	sleep(metadata->planificador->retardo_turno);

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

	printf("%s %d >> Atrapar Pokemon\n", entrenador->nombre_entrenador, entrenador->socket);

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

//	sleep(metadata->planificador->retardo_turno);

	int stock_pokemons = list_size(pokenest->pokemones);

	if ( stock_pokemons > 0) {
		//si hay stock le envio la ruta del pokemon
		t_pokemon * pokemon_capturado = list_get(pokenest->pokemones, 0);
		list_remove(pokenest->pokemones, 0);
		list_add(entrenador->pokemonesCapturados, (void *) pokemon_capturado);

		//actualizo interfaz grafica
//		pthread_mutex_lock(&mutex_gui);
//		restarRecurso(items_mapa, pokenest->identificador);
//		nivel_gui_dibujar(items_mapa, nombreMapa);
//		pthread_mutex_unlock(&mutex_gui);

		char * ruta_pkm = string_new();

		string_append(&(ruta_pkm), "Mapas/");
		string_append(&(ruta_pkm), nombreMapa);
		string_append(&(ruta_pkm), "/PokeNests/");
		string_append(&(ruta_pkm), pokenest->nombre);
		string_append(&(ruta_pkm), "/");
		string_append(&(ruta_pkm), pokemon_capturado->nombreArchivo);

		int ruta_length = string_length(ruta_pkm);

		enviar_header(_CAPTURAR_PKM, ruta_length, entrenador->socket);

		if ( send(entrenador->socket, ruta_pkm, ruta_length, 0) < 0 ) {
			pokemon_remover(pokemon_capturado, entrenador->pokemonesCapturados);
			list_add(pokenest->pokemones, pokemon_capturado);
			return _on_error();
		}

		entrenador->conoce_ubicacion = false;

		t_header * header = recibir_header(entrenador->socket);

		if (header == NULL) {
			pokemon_remover(pokemon_capturado, entrenador->pokemonesCapturados);
			list_add(pokenest->pokemones, pokemon_capturado);
			return _on_error();
		}

		switch (header->identificador) {
			case _QUEDAN_OBJETIVOS:
				//NO HAGO NADA EN ESPECIAL
				entrenador->objetivo_cumplido = false;
				break;
			case _OBJETIVO_CUMPLIDO:
				printf("objetivo cumplido de %s\n", entrenador->nombre_entrenador);
				procesar_objetivo_cumplido(entrenador);
				break;
			default:
				break;
		}

	} else {
		//si no hay stock lo mando a bloqueados de ese pokenest

		printf("NO HAY MAS POKEMONS, EL ENTRENADOR %s se bloqueó\n", entrenador->nombre_entrenador);

		entrenador->bloqueado = true;
		time(&(entrenador->momentoBloqueado));
		queue_push(pokenest->entrenadoresBloqueados, (void *) entrenador);

	}

	quantum_actual = 0;
	keep_running = false;

	return EXIT_SUCCESS;
}

int procesar_objetivo_cumplido(t_entrenador * entrenador) {
	//le envio la ruta de la medalla
	//calculo los tiempos y se los envio

	int datos_size = 0;

	entrenador->objetivo_cumplido = true;

	void * datos = malloc(datos_size);
	//datos:
	//tiempo bloqueado
	//cuantos dl
	//tiempo total en el mapa

	//!!
//
//	enviar_header(_DATOS_FINALES, datos_size, entrenador->socket);
//	send(entrenador->socket, datos, datos_size, 0);

	return EXIT_SUCCESS;
}

//**********************************************************************************

void signal_handler(int signal) {
	if (signal == SIGUSR2) {
		keep_running = false;
		quantum_actual = 0;
		destruir_metadata();
		leer_metadata_mapa(ruta_directorio);
		imprimir_metadata();
	}
}
