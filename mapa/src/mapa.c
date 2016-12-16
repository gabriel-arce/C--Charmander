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
		log_error(logger, "Config NULL en leer_metadata_mapa. Finaliza el programa");
		pthread_mutex_unlock(&mutex_log);
		finalizacionDelPrograma = true;
		finalizarPrograma();
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
			pthread_mutex_lock(&mutex_log);
			log_error(logger, "Algoritmo no reconocido.");
			pthread_mutex_unlock(&mutex_log);
			// se cargo mal el algoritmo -> vuelvo al anterior o default
			metadata->planificador->algth = algoritmo_anterior;
		}
	}

	metadata->planificador->quantum = getIntProperty(conf_file, "quantum");
	metadata->planificador->retardo_turno = getIntProperty(conf_file, "retardo");

	metadata->ip = getStringProperty(conf_file, "IP");
	if (metadata->ip == NULL) log_error(logger, "IP no encontrada");

	metadata->puerto = getIntProperty(conf_file, "Puerto");
	if (&metadata->puerto == NULL) log_error(logger, "Puerto no encontrado");

	cargar_medalla();

	config_destroy(conf_file);
	free(ruta);
	free(algoritmo);

	loguear_metadata();
}

void cargar_medalla() {
	char * ruta_medalla = string_duplicate(ruta_directorio);
	string_append_with_format(&ruta_medalla, "%s/medalla-%s.jpg", nombreMapa, nombreMapa);

	metadata->medallaArchivo = string_duplicate(ruta_medalla);

	free(ruta_medalla);
}

void crear_archivo_log() {
	char * log_name = string_new();
	string_append_with_format(&log_name, "mapa-%s.log", nombreMapa);

	remove((const char *) log_name);
	logger = log_create(log_name, "MAPA log file", false, LOG_LEVEL_TRACE);
	log_info(logger, "MAPA %s iniciado.", nombreMapa);

	free(log_name);
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
		finalizarPrograma();
	}

	// iterate the pokenest folders
	while (1) {
		struct dirent * entry;
		const char * d_name;

		entry = readdir(d);
		if (!entry)
			break;

		d_name = entry->d_name;

			if (strcmp(d_name, "..") != 0 && strcmp(d_name, ".") != 0) {

				char * path = string_new();
				string_append_with_format(&path, "%s/%s", dir_pokenests, d_name);

				//now create the logical pokenest
				t_pokenest * pknst = malloc(sizeof(t_pokenest));
				pknst->nombre = string_duplicate((char *)d_name);
				pknst->pokemones = list_create();
				pknst->posicion = malloc(sizeof(t_posicion));

				//read its files
				DIR * d_pknst = opendir(path);

				if (d_pknst == NULL) {
					pthread_mutex_lock(&mutex_log);
					log_error(logger, "No se pudo abrir el directorio: %s", path);
					pthread_mutex_unlock(&mutex_log);
					finalizarPrograma();
				}

				//primero cargo el metadata del pokenest
				while(1) {
					struct dirent * f_pknst = readdir(d_pknst);

					if (!f_pknst)
						break;

					if ( (strcmp(f_pknst->d_name, "..") == 0) || (strcmp(f_pknst->d_name, ".") == 0) )
						continue;

					char * path_f_pknst = string_new();
					string_append_with_format(&path_f_pknst, "%s/%s", path, f_pknst->d_name);

					if (string_equals_ignore_case(f_pknst->d_name,
							"metadata")) {

						t_config * m_pknst = config_create(path_f_pknst);

						if (m_pknst == NULL) {
							pthread_mutex_lock(&mutex_log);
							log_error(logger,
									"Config NULL en cargar_pokenests() en la ruta %s",
									path_f_pknst);
							pthread_mutex_unlock(&mutex_log);
							finalizarPrograma();
						}

						char * id = getStringProperty(m_pknst, "Identificador");
						//						pknst->identificador = (char) id[0];
						memcpy(&(pknst->identificador), id, 1);
						free(id);

						char * posicion = getStringProperty(m_pknst,
								"Posicion");
						char ** _x_y = string_split(posicion, ";");

						pknst->posicion->x = atoi(_x_y[0]);
						pknst->posicion->y = atoi(_x_y[1]);

						pknst->tipo = getStringProperty(m_pknst, "Tipo");

						free(posicion);
						free(_x_y[0]);
						free(_x_y[1]);
						free(_x_y);
						config_destroy(m_pknst);
						free(path_f_pknst);

						continue;
					}

					free(path_f_pknst);
				}

				rewinddir(d_pknst);

				//despues cargo los pokemons
				while(1) {
					struct dirent * f_pknst = readdir(d_pknst);

					if (!f_pknst)
						break;

					if ( (strcmp(f_pknst->d_name, "..") == 0) || (strcmp(f_pknst->d_name, ".") == 0) )
						continue;

					char * path_f_pknst = string_new();
					string_append_with_format(&path_f_pknst, "%s/%s", path, f_pknst->d_name);

					// if its the metadata go on
					if ( string_equals_ignore_case(f_pknst->d_name, "metadata") ){
						free(path_f_pknst);
						continue;
					}
					// else its a pokemon
					t_config * dat_pkm = config_create(path_f_pknst);

					if (dat_pkm == NULL) {
						pthread_mutex_lock(&mutex_log);
						log_error(logger, "Config NULL en cargar_pokenests()");
						pthread_mutex_unlock(&mutex_log);
						finalizarPrograma();
					}

					if (!config_has_property(dat_pkm, "Nivel")) {
						free(dat_pkm);
						free(path_f_pknst);
						continue;
					}

					t_pkm * pkm = malloc(sizeof(t_pkm));
					pkm->nombre = string_duplicate((char *) d_name);
					pkm->nombreArchivo = string_duplicate(f_pknst->d_name);
					pkm->nivel = getIntProperty(dat_pkm, "Nivel");
					pkm->capturado = false;
					pkm->id_pokenest = pknst->identificador;

					list_add(pknst->pokemones, pkm);

					config_destroy(dat_pkm);
					free(path_f_pknst);
				}

				// <<------ END OF PROCESSING A NEW POKENEST
				ordenar_pokemons(pknst->pokemones);
				list_add(lista_de_pokenests, pknst);
				//Interfaz grafica
				CrearCaja(items_mapa, pknst->identificador, pknst->posicion->x,
						pknst->posicion->y, list_size(pknst->pokemones));

				free(path);
				closedir(d_pknst);
			}
	}

	/* Release everything. */
	if ( closedir(d) == -1 ) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "No se pudo cerrar el directorio: [ %s ]", dir_pokenests);
		pthread_mutex_unlock(&mutex_log);
		finalizarPrograma();
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
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Server-socket() error");
		pthread_mutex_unlock(&mutex_log);
		finalizarPrograma();
	}

	/*"address already in use" error message */
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Server-setsockopt() error");
		pthread_mutex_unlock(&mutex_log);
		finalizarPrograma();
	}

	/* bind */
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(metadata->ip);
	serveraddr.sin_port = htons(metadata->puerto);

	memset(&(serveraddr.sin_zero), '\0', 8);

	if (bind(listener, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) == -1) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Server-bind() error");
		pthread_mutex_unlock(&mutex_log);
		finalizarPrograma();
	}

	/* listen */
	if (listen(listener, 10) == -1) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Server-listen() error");
		pthread_mutex_unlock(&mutex_log);
		finalizarPrograma();
	}

	/* add the listener to the master set */
	FD_SET(listener, &master_fdset);

	/* keep track of the biggest file descriptor */
	fdmax = listener;

	/* loop for new entries */
	while(!finalizacionDelPrograma) {

		read_fds = master_fdset;

		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			pthread_mutex_lock(&mutex_log);
			log_error(logger, "Server-select() error");
			pthread_mutex_unlock(&mutex_log);
			continue;
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
						log_error(logger, "Server-accept() error");
						pthread_mutex_unlock(&mutex_log);
					} else {
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

						free(handshake);
					}

				} else {
					//it's already an existing conection in system
				}
			}
		}
	}
}

int procesar_nuevo_entrenador(int socket_entrenador, int buffer_size) {
	t_entrenador * nuevo_entrenador = recibir_datos_entrenador(socket_entrenador, buffer_size);
//	printf("Nueva conexion del entrenador %c\n", nuevo_entrenador->simbolo_entrenador);

	pthread_mutex_lock(&mutex_log);
	log_trace(logger, "Nueva conexion desde el socket %d, entrenador %c",
			nuevo_entrenador->socket, nuevo_entrenador->simbolo_entrenador);
	pthread_mutex_unlock(&mutex_log);

	if (nuevo_entrenador == NULL)
		return -1;

//	Interfaz grafica
	pthread_mutex_lock(&mutex_gui);
	nivel_gui_dibujar(items_mapa, nombreMapa);
	pthread_mutex_unlock(&mutex_gui);

	pthread_mutex_lock(&mutex_entrenadores);
	list_add(entrenadores_conectados, nuevo_entrenador);
	pthread_mutex_unlock(&mutex_entrenadores);

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
	trainer_sesion->nombre_entrenador = malloc(1 + sizeof(char) * name_size);
	memcpy(trainer_sesion->nombre_entrenador, data_buffer + offset, name_size);
	trainer_sesion->nombre_entrenador[name_size] = '\0';

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
	trainer_sesion->conectado = true;

	//Interfaz grafica
	pthread_mutex_lock(&mutex_gui);
	CrearPersonaje(items_mapa, trainer_sesion->simbolo_entrenador,
			trainer_sesion->posicion->x, trainer_sesion->posicion->y);
	nivel_gui_dibujar(items_mapa, nombreMapa);
	pthread_mutex_unlock(&mutex_gui);

	return trainer_sesion;
}

//*******************************************************************************************

void run_scheduler_thread() {

	while (!finalizacionDelPrograma) {
		waitSemaforo(semaforo_de_listos);
		loguear_cola_de_listos();
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
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Algoritmo desconocido en run_algorithm()");
		pthread_mutex_unlock(&mutex_log);
		break;
	}

	return result;
}

int correr_rr() {

	quantum_actual = metadata->planificador->quantum;
	keep_running = true;
	int result;
	quiere_atrapar = false;

	// Selecciono a un entrenador en la cola de listos
	t_entrenador * entrenador_listo = pop_entrenador();

	if (entrenador_listo == NULL)
		return EXIT_FAILURE;

	entrenador_corriendo = entrenador_listo;

	// Ejecuto al entrenador listo
	while ( keep_running && !cambio_metadata ) {

		pthread_mutex_lock(&mutex_planificador_turno);

		if ( (quantum_actual <= 0) ^ (entrenador_listo->bloqueado) ^ (entrenador_listo->objetivo_cumplido) ) {
			keep_running = false;
			quantum_actual = 0;
			entrenador_corriendo = NULL;
			pthread_mutex_unlock(&mutex_planificador_turno);
			continue;
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
	if ( (entrenador_listo != NULL) && (entrenador_listo->conectado) ) {
		if ( (!quiere_atrapar) && (!(entrenador_listo->bloqueado)) && (!(entrenador_listo->objetivo_cumplido)) && (result != EXIT_FAILURE) ) {
			if (agregar_a_cola(entrenador_listo, cola_de_listos, mutex_cola_listos) != -1)
				signalSemaforo(semaforo_de_listos);
		}
	}


	if (cambio_metadata) {
		cambio_metadata = false;
		releer_metadada();
	}

	entrenador_corriendo = NULL;

	return EXIT_SUCCESS;
}

int correr_srdf() {
	int cantidad_en_listos = list_size(cola_de_listos);
	int i;
	int result = EXIT_SUCCESS;
	keep_running = true;
	//TODO VER EL TEMA DE "quiere_atrapar"

	void una_operacion(t_entrenador * ent) {

		pthread_mutex_lock(&mutex_planificador_turno);
		result = trainer_handler(ent);

		if (result == EXIT_FAILURE) {
			keep_running = false;
		}

		pthread_mutex_unlock(&mutex_planificador_turno);
	}

	// Paso 1: Atiedo una sola operacion de a aquellos que no
	// conozcan su distancia a la proxima pokenest
	for (i = 0; i < cantidad_en_listos; i++) {
		t_entrenador * e = list_get(cola_de_listos, i);

		if ( !(e->conoce_ubicacion) ) {
			una_operacion(e);
			list_remove(cola_de_listos, i);
			agregar_a_cola(e, cola_de_listos, mutex_cola_listos);
			i--;
		}
	}

	if(cambio_metadata) {
		cambio_metadata = false;
		releer_metadada();
		return EXIT_SUCCESS;
	}

	// Paso 2: Si ya no hay entrenador sin conocer su proxima ubicacion
	// ordeno la cola de listos segun el calculo de distancias y saco el primero
	t_entrenador * entrenador = calcularSRDF();
	entrenador_corriendo = entrenador;

	while (keep_running && !cambio_metadata) {
		if ( (entrenador->bloqueado) || (entrenador->objetivo_cumplido) ) {
			keep_running = false;
			break;
		} else {
			una_operacion(entrenador);
		}
	}

	if (cambio_metadata) {
		cambio_metadata = false;
		releer_metadada();

		if ( (!(entrenador->bloqueado)) && (!(entrenador->objetivo_cumplido)) && (result != EXIT_FAILURE) ) {
			pthread_mutex_lock(&mutex_cola_listos);
			list_add_in_index(cola_de_listos, 0, entrenador);
			signalSemaforo(semaforo_de_listos);
			pthread_mutex_unlock(&mutex_cola_listos);
		}
	}

	entrenador_corriendo = NULL;

	return result;
}

t_entrenador * calcularSRDF(){

	t_entrenador * entrenadorConMenorDistancia;

	list_sort(cola_de_listos, (void*) entrenadorMasCercaDePokenest);

	entrenadorConMenorDistancia = pop_entrenador();

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

	pthread_mutex_lock(&(entrenador->mutex_entrenador));

	int buffer_size = sizeof(t_header);
	void * buffer_in = malloc(buffer_size);

	int nbytes_recv = recv(entrenador->socket, buffer_in, buffer_size, 0);

	t_header * header = deserializar_header(buffer_in);

	//DESCONEXION DE UN ENTRENADOR
	if (nbytes_recv <= 0) {
		pthread_mutex_unlock(&(entrenador->mutex_entrenador));
		desconexion_entrenador(entrenador, nbytes_recv);
		free(header);
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

	pthread_mutex_unlock(&(entrenador->mutex_entrenador));

	return EXIT_SUCCESS;
}

int desconexion_entrenador(t_entrenador * entrenador, int nbytes_recv) {

	if (entrenador == NULL)
		return -1;

	pthread_mutex_lock(&(entrenador->mutex_entrenador));

	entrenador->conectado = false;

	if ( (nbytes_recv == 0) || (nbytes_recv == -3) ) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Se desconecto el entrenador %c en el socket %d",
				entrenador->simbolo_entrenador, entrenador->socket);
		pthread_mutex_unlock(&mutex_log);
	}

	if (nbytes_recv == -1) {
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Error en el recv desde el socket %d, entrenador %c", entrenador->socket, entrenador->simbolo_entrenador);
		pthread_mutex_unlock(&mutex_log);
	}

	if (nbytes_recv == -2) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "Se desconecto el entrenador %c por haber finalizado en mapa", entrenador->simbolo_entrenador);
		pthread_mutex_unlock(&mutex_log);
	}

	if (nbytes_recv == 1) {
		pthread_mutex_lock(&mutex_log);
		log_info(logger, "[DEADLOCK]: Se desconecto el entrenador %c por haber perdido la batalla", entrenador->simbolo_entrenador);
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
	//TODO VER BIEN EL RACE CONDITION DE ESTO
	//if (nbytes_recv != -2) {
		//pthread_mutex_lock(&mutex_cola_bloqueados);
		sacar_de_bloqueados(entrenador);
		//pthread_mutex_unlock(&mutex_cola_bloqueados);
//	} //else (nbytes_recv == -2) -> el porque ya finalizo en el mapa y ya no está mas en bloqueados

	//liberar pokemons capturados
	liberar_pokemons(entrenador);

	//lo saco de la lista de elementos de la gui y la actualizo
	//Interfaz grafica
	pthread_mutex_lock(&mutex_gui);
	BorrarItem(items_mapa, entrenador->simbolo_entrenador);
	nivel_gui_dibujar(items_mapa, nombreMapa);
	pthread_mutex_unlock(&mutex_gui);

	if (entrenador_corriendo == entrenador) {
		entrenador_corriendo = NULL;
		keep_running = false;
		quantum_actual = 0;
	}

	entrenador_destroyer(entrenador);

	pthread_mutex_unlock(&(entrenador->mutex_entrenador));

	free(entrenador);
	entrenador = NULL;

	return EXIT_FAILURE;
}

int enviar_ubicacion_pokenest(t_entrenador * entrenador, int id_pokenest) {

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

//	Interfaz grafica
	pthread_mutex_lock(&mutex_gui);
	MoverPersonaje(items_mapa, entrenador->simbolo_entrenador, movimiento->x, movimiento->y);
	nivel_gui_dibujar(items_mapa, nombreMapa);
	pthread_mutex_unlock(&mutex_gui);

	free(buffer_in);
	free(movimiento);

	enviar_header(_RESULTADO_OPERACION, 1, entrenador->socket);

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

//	printf("Atrapar (%c, %c)\n", entrenador->simbolo_entrenador, pokenest->identificador);

	pthread_mutex_lock(&mutex_log);
	log_trace(logger, "Entrenador Bloqueado %c - Pokenest %c .", entrenador->simbolo_entrenador, pokenest->identificador);
	pthread_mutex_unlock(&mutex_log);

	time(&(entrenador->momentoBloqueado));

	pthread_mutex_lock(&mutex_cola_bloqueados);
	list_add(cola_de_bloqueados, entrenador_bloqueado);
	pthread_mutex_unlock(&mutex_cola_bloqueados);
	signalSemaforo(semaforo_de_bloqueados);

	quiere_atrapar = true;

	return EXIT_SUCCESS;
}

void imprimir_lista(t_list * l, char * tittle) {
	int i;

	puts(tittle);
	printf("[");
	for (i = 0; i < l->elements_count; i++) {
		t_entrenador * e = list_get(l, i);

		printf(" %c ", e->simbolo_entrenador);
	}
	printf("]\n");
}

void imprimir_bloqueados() {
	int i;

	puts("Cola de bloqueados");
	printf("[");
	for (i = 0; i < cola_de_bloqueados->elements_count; i++) {
		t_bloqueado * b = list_get(cola_de_bloqueados, i);

		printf(" (%c,%c) ", b->entrenador->simbolo_entrenador, b->pokenest->identificador);
	}
	printf("]\n");
}

int procesar_objetivo_cumplido(t_entrenador * entrenador) {

	pthread_mutex_unlock(&(entrenador->mutex_entrenador));
	pthread_mutex_lock(&(entrenador->mutex_entrenador));

	pthread_mutex_lock(&(mutex_log));
	log_trace(logger, "El entrenador %c ha logrado su objetivo.", entrenador->simbolo_entrenador);
	pthread_mutex_unlock(&(mutex_log));

	entrenador->objetivo_cumplido = true;

	//calculo los tiempos y se los envio
	enviar_datos_finales_entrenador(entrenador);

	pthread_mutex_unlock(&(entrenador->mutex_entrenador));

	desconexion_entrenador(entrenador, -2);

	return EXIT_SUCCESS;
}

//**********************************************************************************

void signal_handler(int signal) {

	switch (signal) {

	case SIGUSR2:
		algoritmo_anterior = metadata->planificador->algth;
		if (entrenador_corriendo != NULL) {
			cambio_metadata = true;
		} else {
			releer_metadada();
		}
		break;

	case SIGINT:
		//puts("Finalizando mapa");
		finalizarPrograma();
		break;

	default:
		pthread_mutex_lock(&mutex_log);
		log_error(logger, "Signal sin definir");
		pthread_mutex_unlock(&mutex_log);
	}
}

void releer_metadada() {
	pthread_mutex_lock(&mutex_log);
	log_info(logger, "Cambios en el metadata. Se vuelve a cargar.");
	pthread_mutex_unlock(&mutex_log);
	destruir_metadata();
	leer_metadata_mapa(ruta_directorio);

	if (cola_de_listos->elements_count > 0)
		signalSemaforo(semaforo_de_listos);
}

void atender_bloqueados() {

	int cantidad_bloqueados = 0;
	int i;
	int r;
	t_entrenador * e = NULL;

	while (!finalizacionDelPrograma) {
		waitSemaforo(semaforo_de_bloqueados);

		loguear_cola_de_bloqueados();

		//TODO GUARDA CON ESTO!!!!
		pthread_mutex_lock(&mutex_cola_bloqueados);

		cantidad_bloqueados = list_size(cola_de_bloqueados);

		if (cantidad_bloqueados <= 0) {
			pthread_mutex_unlock(&mutex_cola_bloqueados);
			continue;
		}

		for (i = 0; i < cantidad_bloqueados; i++) {
			t_bloqueado * b = list_get(cola_de_bloqueados, i);

			pthread_mutex_lock(&(b->entrenador->mutex_entrenador));

			if (b == NULL) {
				pthread_mutex_unlock(&(b->entrenador->mutex_entrenador));
				continue;
			}

			if (b->entrenador == NULL || !(b->entrenador->conectado)) {
				list_remove(cola_de_bloqueados, i);
				i--;
				pthread_mutex_unlock(&(b->entrenador->mutex_entrenador));
				free(b);
				b = NULL;
				continue;
			}

			t_pkm * pkm = obtener_primer_no_capturado(b->pokenest);
			r = EXIT_SUCCESS;
			e = b->entrenador;

			if (pkm != NULL /*pudo capturar*/) {
				list_remove(cola_de_bloqueados, i);
				i--;
				r = generar_captura(b->entrenador, b->pokenest, pkm);
				free(b);
				b = NULL;
				cantidad_bloqueados--;
			}

			if (r != EXIT_FAILURE) {
				if (e != NULL && e->conectado)
					pthread_mutex_unlock(&(e->mutex_entrenador));
			}

			e = NULL;
		}

		pthread_mutex_unlock(&mutex_cola_bloqueados);

	}

}

int generar_captura(t_entrenador * entrenador, t_pokenest * pokenest, t_pkm * pokemon) {

	//	Interfaz grafica
	pthread_mutex_lock(&mutex_gui);
	restarRecurso(items_mapa, pokenest->identificador);
	nivel_gui_dibujar(items_mapa, nombreMapa);
	pthread_mutex_unlock(&mutex_gui);

	int _on_error() { //failed to send pkm
		pokemon->capturado = false;
		pthread_mutex_lock(&mutex_log);
		log_error(logger,
				"Error al tratar de enviar el pokemon al entrenador %c .",
				entrenador->simbolo_entrenador);
		pthread_mutex_unlock(&mutex_log);
		pthread_mutex_unlock(&(entrenador->mutex_entrenador));
		desconexion_entrenador(entrenador, -2);
		//interfaz grafica
		incrementar_recurso(pokenest->identificador);
		return EXIT_FAILURE;
	}

	pokemon->capturado = true;

	//LE ENVIA EL POKEMON AL ENTRENADOR
	if (serializarYEnviarPokemon(_CAPTURAR_PKM, pokemon, entrenador->socket) == EXIT_FAILURE)
		return _on_error();

	time_t tiempo_desbloqueo;
	time(&tiempo_desbloqueo);

	list_add(entrenador->pokemonesCapturados, pokemon);
	entrenador->conoce_ubicacion = false;
	entrenador->tiempoBloqueado += difftime(tiempo_desbloqueo, entrenador->momentoBloqueado);
	entrenador->momentoBloqueado = 0;

	pthread_mutex_lock(&mutex_log);
	log_trace(logger, "El entrenador %c atrapo a %s .", entrenador->simbolo_entrenador, pokemon->nombre);
	pthread_mutex_unlock(&mutex_log);

	t_header * header = recibir_header(entrenador->socket);

	if (header == NULL)
		return _on_error();

	switch (header->identificador) {
		case _QUEDAN_OBJETIVOS:
			entrenador->objetivo_cumplido = false;
			entrenador->bloqueado = false;
			if (agregar_a_cola(entrenador, cola_de_listos, mutex_cola_listos) != -1)
				signalSemaforo(semaforo_de_listos);
			break;
		case _OBJETIVO_CUMPLIDO:
			procesar_objetivo_cumplido(entrenador);
			break;
		default:
			break;
	}

	free(header);

	return EXIT_SUCCESS;
}

int enviar_datos_finales_entrenador(t_entrenador * entrenador) {

	//calculo los tiempos y se los envio
	time_t tiempo_actual;
	time(&(tiempo_actual));
	double tiempo_tot_mapa = difftime(tiempo_actual,
			entrenador->tiempoDeIngresoAlMapa); //[seg]

	int datos_size = 2 * sizeof(double) + sizeof(int);

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

	if (enviar_header(_DATOS_FINALES, datos_size, entrenador->socket) == -1)
		return -1;

	if (send(entrenador->socket, datos, datos_size, 0) == -1)
		return -1;
//
//	puts("*******DATOS QUE ENVIO*******");
//	printf("ENTRENADOR: %c(%s)\n", entrenador->simbolo_entrenador, entrenador->nombre_entrenador);
//	printf("total: %f\n", tiempo_tot_mapa);
//	printf("tiempo en bloqueado: %f\n", entrenador->tiempoBloqueado);
//	printf("DLs involucrado: %d\n", entrenador->deadlocksInvolucrados);
//
	free(datos);

	return 0;
}

void finalizarPrograma(){

	pthread_mutex_lock(&mutex_log);
	log_trace(logger, "El mapa %s ha finalizado.", nombreMapa);
	pthread_mutex_unlock(&mutex_log);

	destruir_variables();
	destruir_metadata();
	destruir_semaforos();
	finalizacionDelPrograma = true;

	pthread_detach(hilo_planificador);
	pthread_detach(hilo_servidor);
	pthread_detach(hilo_bloqueados);
	pthread_detach(hilo_deadlock);

	nivel_gui_terminar();

	exit(EXIT_SUCCESS);
}
