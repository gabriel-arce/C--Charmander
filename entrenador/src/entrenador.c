/*
 ============================================================================
 Name        : entrenador.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "entrenador.h"

t_mapa * crear_mapa(char * nombre_mapa) {
	t_mapa * mapa = malloc(sizeof(t_mapa));
	memset(mapa, 0, sizeof(t_mapa));

	mapa->nombre_mapa = string_duplicate(nombre_mapa);
	mapa->objetivos = queue_create();

	return mapa;
}

t_metadata_entrenador * crear_metadata() {
	t_metadata_entrenador * metadata = malloc(sizeof(t_metadata_entrenador));
	memset(metadata, 0, sizeof(t_metadata_entrenador));

	metadata->viaje = queue_create();

	return metadata;
}

int leer_metadata_entrenador(char * metada_path) {
	metadata = crear_metadata();

	t_config * conf_file = config_create(metada_path);

	if (conf_file == NULL) {
		printf("No se pudo leer el metadata: Error en el nombre del entrenador o ruta incorrecta.\n");
		free(metadata);
		free(metada_path);
		free(nombreEntrenador);
		exit(EXIT_FAILURE);
	}

	metadata->nombre = getStringProperty(conf_file, "nombre");

	metadata->simbolo = (char) getStringProperty(conf_file, "simbolo")[0];

	t_list * hoja_de_viaje = getListProperty(conf_file, "hojaDeViaje");

	if (hoja_de_viaje == NULL){
		finalizacionAbrupta();
	}

	int ciudades = list_size(hoja_de_viaje);
	printf("%d", ciudades);

	int i;
	for (i = 0; i < ciudades; i++) {
		char * ciudad = list_get(hoja_de_viaje, i);
		printf("%s", ciudad);
		t_mapa * mapa_i = crear_mapa(ciudad);

		char * property_name = string_duplicate("obj[");
		string_append(&(property_name), ciudad);
		string_append(&(property_name), "]");

		mapa_i->objetivos->elements = getListProperty(conf_file, property_name);

		list_add_in_index(metadata->viaje->elements, i, mapa_i);

		free(property_name);

	}

	list_destroy_and_destroy_elements(hoja_de_viaje, (void *) free);

	metadata->vidas = getIntProperty(conf_file, "vidas");

	config_destroy(conf_file);

	return 0;
}

void imprimir_metadata() {
	printf("\n >>> METADATA ENTRENADOR <<<\n");
	printf("nombre: %s\n", metadata->nombre);
	printf("simbolo: %c\n", metadata->simbolo);
	printf("vidas: %d\n", metadata->vidas);

	printf("\n**Hoja de viaje**\n");

	int ciudades = list_size(metadata->viaje->elements);
	int i;

	for(i = 0; i < ciudades; i++) {
		t_mapa * mapa = list_get(metadata->viaje->elements, i);
		printf("%s\n", mapa->nombre_mapa);
		printf("objetivos: [");
		int objetivos = list_size(mapa->objetivos->elements);
		int j;
		for (j = 0; j < objetivos; j++) {
			printf(" %s ", (char *) list_get(mapa->objetivos->elements, j));
		}
		printf("]\n");
	}

	printf("\n");
}

int conectarse_a_un_mapa(int puerto, char * ip) {
	int socket_fd = -1;

	socket_fd = clienteDelServidor(ip, puerto);

	if (socket_fd == -1) {
		puts("Error al intentar conectarse a un mapa");
		finalizacionAbrupta();
	}

	return socket_fd;
}

int enviar_datos_a_mapa(int socket, char simbolo, char * nombre) {

	int buffer_size = sizeof(int) + string_length(nombre);
	void * data_buffer = malloc(buffer_size);

	if(enviar_header(1, buffer_size, socket) < 0){
		puts("Error al enviar header de datos");
		finalizacionAbrupta();
	}

	int num_simbolo = (int) simbolo;

	memcpy(data_buffer, &(num_simbolo), sizeof(int));
	memcpy(data_buffer + sizeof(int), nombre, string_length(nombre));

	if (send(socket, data_buffer, buffer_size, 0) < 0)
		return -1;

	free(data_buffer);

	return EXIT_SUCCESS;
}


//------------------Señales

void rutina(int signal){
	switch(signal){

		case SIGUSR1:
			metadata->vidas += 1;
			puts("Se ha agregado una vida al entrenador");
			printf("El entrenador tiene: %d vidas \n", metadata->vidas);
			break;

		case SIGTERM:
			if(metadata->vidas > 0){
				metadata->vidas --;
				puts("Se ha quitado una vida al entrenador");
				printf("El entrenador tiene: %d vidas \n", metadata->vidas);
			}
			else {
				puts("El entrenador no tiene mas vidas!");
				muereEntrenador = true;
				if(estoyBloqueado){
					enviar_header(_DESCONEXION, 0, socket_entrenador);
					estoyBloqueado = false;
//					close(socket_entrenador);
//					socket_entrenador = -1;
				}
			}
			break;

		case SIGINT:
			puts("\nMuerte forzosa. Bye...");
			enviar_header(_DESCONEXION, 0, socket_entrenador);
			limpiar_pokemons_en_directorio();
			rm_de_medallas();
			finalizarEntrenador();
			liberarRecursos();
			exit(EXIT_SUCCESS);
			break;

		default: puts("Codigo de señal invalida");
	}

}

//-------------------Funciones

void inicializarEntrenador(){
	inicializarSinmuertesNiReintentos();
	setRutaMedallas();
	setRutaDirDeBill();
	cantidadDeMuertes = 0;
	tiempoDeJuego = 0;
	tiempoBloqueado = 0;
	deadlocksInvolucrados = 0;
	reintentos = 0;
}

void inicializarSinmuertesNiReintentos(){
	cargarMetadata();
	socket_entrenador = -1;
	ubicacionProximaPokenest = malloc(sizeof(t_posicion));
	pokemonesCapturados = list_create();
	pokemonMasFuerte = NULL;
	finDelJuego = false;
	muereEntrenador = false;
	ubicacionActual = malloc(sizeof(t_posicion));
	estoyBloqueado = false;
}

void cargarMetadata(){

	char * ruta = string_duplicate(pokedex_path);

	string_append_with_format(&ruta, "Entrenadores/%s/metadata", nombreEntrenador);

	leer_metadata_entrenador(ruta);

	imprimir_metadata();

	free(ruta);
}

void conectarseConSiguienteMapa(){

	t_mapa * mapa_peek = (t_mapa *) queue_peek(metadata->viaje);
	mapaActual = copiarMapa(mapa_peek);

	if (mapaActual == NULL) {
		printf("\nerror\n");
		finalizacionAbrupta();
	}

	cargar_mapa();

	socket_entrenador = conectarse_a_un_mapa(mapaActual->puerto,
			mapaActual->ip);

	if (socket_entrenador < 0) {

		printf("No es posible conectarse con el mapa: %s \n",mapaActual->nombre_mapa);
		finalizacionAbrupta();

	} else {

		printf("Conectado con %s \n", mapaActual->nombre_mapa);

		if(enviar_datos_a_mapa(socket_entrenador, metadata->simbolo,metadata->nombre) == -1){
			puts("Error al enviar datos a mapa");
			finalizacionAbrupta();
		}

		ubicacionActual->x = 0;
		ubicacionActual->y = 0;
		pokenestLocalizada = false;
	}
}

t_mapa *  copiarMapa(t_mapa * mapaACopiar){
	t_mapa * mapaCopiado = malloc(sizeof(t_mapa));
//	memcpy(&(mapaCopiado->socket), &(mapaACopiar->socket), 4);
	mapaCopiado->nombre_mapa = string_duplicate(mapaACopiar->nombre_mapa);
	mapaCopiado->objetivos = queue_create();

	int i;
	char * obj = NULL;
	char * obj_copy = NULL;
	for(i = 0; i < mapaACopiar->objetivos->elements->elements_count; i++) {
		obj = (char *) list_get(mapaACopiar->objetivos->elements, i);
		obj_copy = string_duplicate(obj);

		list_add(mapaCopiado->objetivos->elements, obj_copy);
	}

//	memcpy(mapaCopiado,mapaACopiar,sizeof(t_mapa));
	return mapaCopiado;
}


void cargar_mapa() {
	char * ruta_del_mapa = string_duplicate(pokedex_path);
	string_append_with_format(&ruta_del_mapa, "Mapas/%s/metadata", mapaActual->nombre_mapa);

	t_config * m_mapa = config_create(ruta_del_mapa);

	mapaActual->ip = string_duplicate(getStringProperty(m_mapa, "IP"));
	mapaActual->puerto = getIntProperty(m_mapa, "Puerto");

	free(ruta_del_mapa);
	config_destroy(m_mapa);
}

void solicitarUbicacionDelProximoPokenest(){
	char * id_pokemon;

	id_pokemon = queue_pop(mapaActual->objetivos);

	enviarSolicitudUbicacionPokenest(id_pokemon[0]);
	free(id_pokemon);
}

void enviarSolicitudUbicacionPokenest(char id_pokemon){

	if (enviar_header(_UBICACION_POKENEST, (int) id_pokemon, socket_entrenador) < 0){
		puts("Error al enviar header de ubicacion de pokenest");
		finalizacionAbrupta();
		}

	int tamanio_coord = sizeof(t_posicion);
	void * coordenadas = malloc(tamanio_coord);

	if(recv(socket_entrenador, coordenadas, tamanio_coord, 0) < 0) {
		puts("Error en el recv de _UBICACION_POKENEST");
		finalizacionAbrupta();
	}

	memcpy(&(ubicacionProximaPokenest->x), coordenadas, 4);
	memcpy(&(ubicacionProximaPokenest->y), coordenadas + 4, 4);

	if ((ubicacionProximaPokenest->x == -1)||(ubicacionProximaPokenest->y == -1)) {
		puts("Error en las coordenadas recibidas");
		finalizacionAbrupta();
	}

	pokenestLocalizada = true;

	free(coordenadas);
}

void avanzarHaciaElPokenest(){

	int movimientosRestantesEnX = (ubicacionProximaPokenest->x - ubicacionActual->x);
	int movimientosRestantesEnY = (ubicacionProximaPokenest->y - ubicacionActual->y);

	if(abs(movimientosRestantesEnX) >= abs(movimientosRestantesEnY)){
		if(movimientosRestantesEnX > 0) ubicacionActual->x += 1;
		if(movimientosRestantesEnX < 0) ubicacionActual->x -= 1;
	}
	else{
		if(movimientosRestantesEnY > 0) ubicacionActual->y += 1;
		if(movimientosRestantesEnY < 0) ubicacionActual->y -= 1;
	}

	enviarUbicacionAMapa();

	t_header * header;
	header = recibir_header(socket_entrenador);

	if(header == NULL){
		puts("Error al recibir confirmacion de avanzar");
		finalizacionAbrupta();
				}
	free(header);
}

void enviarUbicacionAMapa(){

	if(enviar_header(_MOVER_XY, sizeof(t_posicion), socket_entrenador) < 0){
		puts("Error al enviar header de ubicacion a mapa");
		finalizacionAbrupta();
				}

	void* buffer_out = malloc(sizeof(t_posicion));
	memcpy(buffer_out,&(ubicacionActual->x),4);
	memcpy(buffer_out + 4,&(ubicacionActual->y),4);

	if(send(socket_entrenador,buffer_out,sizeof(t_posicion),0) < 0 ){
		puts("Error al enviar ubicacion");
		finalizacionAbrupta();
				}
	free(buffer_out);
}

void atraparPokemon(){

	if (enviar_header(_CAPTURAR_PKM,0,socket_entrenador) < 0){
		puts("Error al enviar header de capturar pokemon");
		finalizacionAbrupta();
	}

	estoyBloqueado = true;

	//escuchar y hay dos posibilidades, (deadlock y vuelvo a escuchar) o (pokemon y sigo la rutina)

	t_header * header_in = NULL;
	bool pokemonCapturadoOEntrenadorMuerto = false;

	while (!pokemonCapturadoOEntrenadorMuerto) {
		header_in = recibir_header(socket_entrenador);

		if (header_in == NULL) {
			if(estoyBloqueado == false){
				//retorna porque murio por consecuencia de la señal
				return;
			}
			else{
				puts("Error al recibir header");
				finalizacionAbrupta();
			}
		}

		switch (header_in->identificador) {
		case _CAPTURAR_PKM:

			pokemonCapturadoOEntrenadorMuerto = true;

			t_pkm * pokemonAtrapado = recibirYDeserializarPokemon(socket_entrenador, header_in->tamanio);

			if (pokemonAtrapado == NULL) {
				finalizacionAbrupta();
			}

			//PRINTEO PKM PARA TEST
			printf("Nombre: %s\n", pokemonAtrapado->nombre);
			printf("Archivo: %s\n", pokemonAtrapado->nombreArchivo);
			printf("Nivel: %d\n", pokemonAtrapado->nivel);
			printf("Capturado: %s\n", pokemonAtrapado->capturado ? "true" : "false");
			printf("ID_PKNST: %c\n", pokemonAtrapado->id_pokenest);

			list_add(pokemonesCapturados, pokemonAtrapado);

			puts("pkm capturado");

			copiarPokemon(pokemonAtrapado);

			estoyBloqueado = false;
			verificarNivelPokemon(pokemonAtrapado);
			verificarSiQuedanObjetivosEnMapa();
			pokenestLocalizada = false;

			break;

		case _BATALLA:
			pokemonCapturadoOEntrenadorMuerto = batallaPokemon();
			break;

		default:
			puts("identificador de header invalido");
			finalizacionAbrupta();
			break;
		}

		free(header_in);
	}
}

void verificarSiQuedanObjetivosEnMapa(){
	if (queue_size(mapaActual->objetivos) == 0) {
		if(enviar_header(_OBJETIVO_CUMPLIDO, 0, socket_entrenador) < 0){
			puts("Error al enviar header de objetivo cumplido");
			finalizacionAbrupta();
					}

		recibirDatosFinales();

		copiarMedalla();
		desconectarseDeMapa();

		t_mapa* mapa = queue_pop(metadata->viaje);
		liberarMapa(mapa);

		if (queue_size(metadata->viaje) == 0) {
			finDelJuego = true;
		} else {
			conectarseConSiguienteMapa();
		}
	} else {

		if(enviar_header(_QUEDAN_OBJETIVOS, 0, socket_entrenador) < 0){
			puts("Error al enviar header de quedan objetivos");
			finalizacionAbrupta();
					}
	}
}

void recibirDatosFinales(){
	t_header * header = recibir_header(socket_entrenador);

			if(header == NULL){
				puts("Error al recibir header de los datos finales");
				finalizacionAbrupta();
						}

			void * datos = malloc(header->tamanio);

			if(recv(socket_entrenador, datos, header->tamanio, 0) < 0){
				puts("Error al recibir los datos finales");
				finalizacionAbrupta();
						}

			procesarDatos(datos);
			free(datos);
			free(header);
}

void procesarDatos(void * datos) {
	double tiempo_en_el_mapa = 0;
	double tiempo_bloqueado = 0;
	int dls_involucrado = 0;

	int offset = sizeof(double);
	memcpy(&(tiempo_en_el_mapa), datos, offset);
	memcpy(&(tiempo_bloqueado), datos + offset, offset);
	offset += sizeof(double);
	memcpy(&(dls_involucrado), datos + offset, sizeof(int));

	tiempoDeJuego += tiempo_en_el_mapa;
	tiempoBloqueado += tiempo_bloqueado;
	deadlocksInvolucrados += dls_involucrado;
}

void setRutaMedallas() {
	rutaMedallas = string_duplicate(pokedex_path);
	string_append_with_format(&rutaMedallas, "Entrenadores/%s/medallas/", nombreEntrenador);
}

void copiarMedalla() {
	char * ruta_medalla = string_duplicate(pokedex_path);
	string_append_with_format(&(ruta_medalla), "Mapas/%s/medalla-%s.jpg\0",
			mapaActual->nombre_mapa, mapaActual->nombre_mapa);

	copiar_archivo(ruta_medalla, rutaMedallas);

	free(ruta_medalla);
}

void copiarPokemon(t_pkm * pokemonAtrapado) {
	char * ruta_pkm = string_new();
	string_append_with_format(&ruta_pkm, "%sMapas/%s/PokeNests/%s/%s\0", pokedex_path,
			mapaActual->nombre_mapa, pokemonAtrapado->nombre,
			pokemonAtrapado->nombreArchivo);

	copiar_archivo(ruta_pkm, rutaDirDeBill);

	free(ruta_pkm);
}

char * obtener_nombre_pokemon(char * ruta) {
	char * nombre;

	char ** split = string_split(ruta, "/");
	int i = 0;

	while (!(string_equals_ignore_case(split[i], "PokeNests"))) {
		i++;
	}

	nombre = string_duplicate(split[i + 1]);

	i = 0;
	while (split[i] != NULL) {
		free(split[i]);
		i++;
	}
	free(split);

	return nombre;
}

void setRutaDirDeBill() {
	rutaDirDeBill = string_duplicate(pokedex_path);
	//TODO cuidado cuando tengamos el fs posta
	string_append_with_format(&rutaDirDeBill, "Entrenadores/%s/Dir%s de%s Bill/", nombreEntrenador, "\\", "\\");
}

void verificarNivelPokemon(t_pkm * pokemon){

	if (pokemonMasFuerte == NULL)
		pokemonMasFuerte = pokemon;

	if(pokemon->nivel > pokemonMasFuerte->nivel){

		pokemonMasFuerte = pokemon;

	}
}

void realizarAccion(){

	if(!pokenestLocalizada){
		puts("solicitar ubicacion");
		solicitarUbicacionDelProximoPokenest();
		return;
	}

	if(estoyEnPokenest()){
		puts("atrapar pokemon");
		atraparPokemon();
		return;
	}

	if(!estoyEnPokenest()){
		puts("avanzar");
		avanzarHaciaElPokenest();
		return;
	}

}



bool estoyEnPokenest() {

	if (ubicacionProximaPokenest == NULL)
		return false;

	if (ubicacionActual == NULL)
		return false;

	return (ubicacionProximaPokenest->x == ubicacionActual->x) && (ubicacionProximaPokenest->y == ubicacionActual->y);
}

void imprimirLogro(){

	puts("***************************--------***************************");
	printf("%s: Te has convertido en un maestro pokemon!\n", nombreEntrenador);
	printf("El tiempo total de tu aventura fue de: %f segundos \n ", tiempoDeJuego);
	printf("Pasaste %f segundos bloqueado \n", tiempoBloqueado);
	printf("Estuviste involucrado en %d deadlocks \n", deadlocksInvolucrados);
	printf("Has muerto %d veces \n", cantidadDeMuertes);

}

bool batallaPokemon(){ 					//retorna true si muere

//	enviarPokemon(pokemonMasFuerte,  socket_entrenador);
	if ( serializarYEnviarPokemon(_PKM_MAS_FUERTE, pokemonMasFuerte, socket_entrenador) == EXIT_FAILURE ) {
		puts("Error en el envio del pokemon mas fuerte.");
		finalizacionAbrupta();
	}

	t_header * header = recibir_header(socket_entrenador); //mapa envia si entrenador gana la batalla o no

	if(header == NULL){
		puts("Error al recibir el header de respuesta de batalla");
		finalizacionAbrupta();
				}

	if (header->identificador != _RESULTADO_BATALLA) {

		puts("identificador de header desconocido");
		finalizacionAbrupta();
	} else {

		if (header->tamanio == 0) {
			recibirDatosFinales();
			puts("El entrenador ha perdido una batalla pokemon");
			muereEntrenador = true;
			return true;
		} else {
			puts("El entrenador ha ganado la batalla pokemon");
		}
	}
	free(header);
	return false;
}

void muerteEntrenador() {
	muereEntrenador = false;
	cantidadDeMuertes += 1;
	puts("El entrenador a muerto");

	enviar_header(_DESCONEXION, 0, socket_entrenador);

	if(metadata->vidas > 0){
		metadata->vidas -= 1;
		desconectarseDeMapa();
		//Se reconecta al mismo mapa
		conectarseConSiguienteMapa();
	}
	else{
		char  respuesta;
		bool contestaConOtroCaracter = true;

		printf("No quedan vidas disponibles, cantidad de reintentos: %d \n", reintentos);
		puts("¿Desea reiniciar el juego?");
		puts("1/0");

		while(contestaConOtroCaracter){
//		scanf(respuesta);
		respuesta = fgetc(stdin);

		switch(respuesta){

		case '1':
			reintentos += 1;
			rm_de_medallas();
			desconectarseDeMapa();
			finalizarEntrenador();
			inicializarSinmuertesNiReintentos();
			conectarseConSiguienteMapa();
			contestaConOtroCaracter = false;
			break;

		case '0':
			contestaConOtroCaracter = false;
			limpiar_pokemons_en_directorio();
			rm_de_medallas();
			finalizarEntrenador();
			liberarRecursos();
			exit(EXIT_SUCCESS);
			break;

		default:
			puts("conteste con 1/0");
		}
		}
	}

}

void pokemon_destroyer(t_pkm * p) {
	free(p->nombre);
	free(p->nombreArchivo);
	free(p);
}

void desconectarseDeMapa(){
//	if(socket_entrenador != -1)
//		close(socket_entrenador);
	limpiar_pokemons_en_directorio();
	list_clean_and_destroy_elements(pokemonesCapturados, (void *) pokemon_destroyer);
	pokenestLocalizada = false;

	if(pokemonMasFuerte)
		pokemonMasFuerte = NULL;

	if(mapaActual != NULL)
		liberarMapa(mapaActual);

	mapaActual = NULL;
}

void rm_pokemon(char * dir_pkm) {
	char * comando = string_duplicate("rm -f ");
	string_append(&(comando), dir_pkm);

	system(comando);

	free(comando);
}

void rm_de_pokemons() {
	char * comando = string_duplicate("rm -f ");
	string_append(&(comando), rutaDirDeBill);
	string_append(&(comando), "*");

	system(comando);

	free(comando);
}

void limpiar_pokemons_en_directorio() {
//	int i;
//	int pkms = list_size(pokemonesCapturados);
//
//	for (i = 0; i < pkms; i++) {
//		t_pkm * p = list_get(pokemonesCapturados, i);
//
//		//TODO CAMBIAR A SIMBOLO DEL MAPA
//		if (p->mapa == mapaActual->socket)
//			rm_pokemon(p->nombreArchivo);
//	}
	rm_de_pokemons();
}

void rm_de_medallas() {
	char * comando = string_duplicate("rm -f ");
	string_append(&(comando), rutaMedallas);
	string_append(&(comando), "*");

	system(comando);

	free(comando);
}

void finalizarEntrenador(){

	if(metadata->nombre != NULL)
		free(metadata->nombre);

	if(ubicacionActual != NULL)
		free(ubicacionActual);

	if(metadata->viaje != NULL)
		queue_destroy_and_destroy_elements(metadata->viaje, (void*) liberarMapa);

	if (ubicacionProximaPokenest != NULL)
		free(ubicacionProximaPokenest);

	if (mapaActual != NULL)
		free(mapaActual);

	if (pokemonMasFuerte != NULL)
		free(pokemonMasFuerte);

	if(metadata != NULL)
		free(metadata);

}

void destruirHojaDeViaje(t_mapa * mapa){
	free(mapa->nombre_mapa);
	queue_clean_and_destroy_elements(mapa->objetivos, (void *) free);
	free(mapa);
}

void copiar_archivo(char * source, char * destination) {
	char * comando = string_new();
	string_append_with_format(&comando, "cp %s %s", source, destination);

	system(comando);

	free(comando);
}

char * generar_ruta_archivo(char * ruta) {
	char * path;

	path = string_duplicate(rutaDirDeBill);

	//desarmo la ruta para sacar el nombre de archivo
	char ** aux = string_split(ruta, "/");

	int i = 0;
	while (aux[i] != NULL) {
		i++;
	}

	string_append(&path, aux[i -1]);

	i = 0;
	while (aux[i] != NULL) {
		free(aux[i]);
		i++;
	}

	free(aux);

	return path;
}

void liberarRecursos(){
	if(pokedex_path != NULL)
		free(pokedex_path);

	if(nombreEntrenador != NULL)
		free(nombreEntrenador);

	if(rutaMedallas != NULL)
		free(rutaMedallas);

	if(rutaDirDeBill != NULL)
		free(rutaDirDeBill);
}

void finalizacionAbrupta(){
	desconectarseDeMapa();
	rm_de_medallas();
	finalizarEntrenador();
	liberarRecursos();
	exit(EXIT_FAILURE);
}

void liberarMapa(t_mapa* mapa){
	free(mapa->ip);
	free(mapa->nombre_mapa);
	queue_destroy_and_destroy_elements(mapa->objetivos, (void*) free);
	free(mapa);
}
