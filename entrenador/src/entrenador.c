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

	metadata->nombre = getStringProperty(conf_file, "nombre");

	metadata->simbolo = (char) getStringProperty(conf_file, "simbolo")[0];

	t_list * hoja_de_viaje = getListProperty(conf_file, "hojaDeViaje");

	if (hoja_de_viaje == NULL)
		return -1;

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
	metadata->reintentos = getIntProperty(conf_file, "reintentos");

	free(conf_file);

	return 0;
}

void imprimir_metadata() {
	printf("\n >>> METADATA ENTRENADOR <<<\n");
	printf("nombre: %s\n", metadata->nombre);
	printf("simbolo: %c\n", metadata->simbolo);
	printf("vidas: %d\n", metadata->vidas);
	printf("reintentos: %d\n", metadata->reintentos);

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

	return socket_fd;
}

int enviar_datos_a_mapa(int socket, char simbolo, char * nombre) {

	int buffer_size = sizeof(int) + string_length(nombre);
	void * data_buffer = malloc(buffer_size);

	enviar_header(1, buffer_size, socket);

	int num_simbolo = (int) simbolo;

	memcpy(data_buffer, &(num_simbolo), sizeof(int));
	memcpy(data_buffer + sizeof(int), nombre, string_length(nombre));

	if (send(socket, data_buffer, buffer_size, 0) < 0)
		return -1;

	return EXIT_SUCCESS;
}


//------------------Señales

void rutina(int signal){
	switch(signal){

		case SIGUSR1:
			metadata->vidas += 1;
			puts("Se ha agregado una vida al entrenador");
			break;

		case SIGTERM:

			muereEntrenador = true;
			puts("Se ha quitado una vida al entrenador");
			break;

		default: puts("Codigo de señal invalida");
	}

	printf("El entrenador tiene: %d vidas \n", metadata->vidas);
}

//-------------------Funciones

void inicializarEntrenador(){
	cargarMetadata();
	cantidadDeMuertes = 0;
	metadata->reintentos = 0;
	inicializarSinmuertesNiReintentos();
}

void inicializarSinmuertesNiReintentos(){

	socket_entrenador = -1;
	ubicacionProximaPokenest = malloc(sizeof(t_posicion));
	pokemonesCapturados = list_create();
	pokemonMasFuerte = NULL;
	finDelJuego = false;
	muereEntrenador = false;
	ubicacionActual = malloc(sizeof(t_posicion));
}
void cargarMetadata(){

	char * ruta = string_duplicate(metadata_path);

	string_append(&ruta,"Entrenadores/");
	string_append(&ruta,nombreEntrenador);
	string_append(&ruta,"/metadata/metadata");

	leer_metadata_entrenador(ruta);

	imprimir_metadata();

	free(ruta);
}

void conectarseConSiguienteMapa(){

	mapaActual = queue_peek(metadata->viaje);

	if (mapaActual == NULL) {
		printf("\nerror\n");
		exit(1);
	}

	cargar_mapa();

	socket_entrenador = conectarse_a_un_mapa(mapaActual->puerto,
			mapaActual->ip);

	if (socket_entrenador < 0) {

		printf("No es posible conectarse con el mapa: %s \n",
				mapaActual->nombre_mapa);

	} else {

		printf("Conectado con %s \n", mapaActual->nombre_mapa);

		enviar_datos_a_mapa(socket_entrenador, metadata->simbolo,
				metadata->nombre);

		ubicacionActual->x = 0;
		ubicacionActual->y = 0;
		pokenestLocalizada = false;
	}
}

void cargar_mapa() {
	char * ruta_del_mapa = string_duplicate(metadata_path);
	string_append(&(ruta_del_mapa), "Mapas/");
	string_append(&(ruta_del_mapa), mapaActual->nombre_mapa);
	string_append(&(ruta_del_mapa), "/metadata");

	t_config * m_mapa = config_create(ruta_del_mapa);

	mapaActual->ip = string_duplicate(getStringProperty(m_mapa, "IP"));
	mapaActual->puerto = getIntProperty(m_mapa, "Puerto");

	free(ruta_del_mapa);
	free(m_mapa);
}

void solicitarUbicacionDelProximoPokenest(){
	char * id_pokemon;

	id_pokemon = queue_pop(mapaActual->objetivos);

	enviarSolicitudUbicacionPokenest(id_pokemon[0]);
}

void enviarSolicitudUbicacionPokenest(char id_pokemon){

	enviar_header(_UBICACION_POKENEST, (int) id_pokemon, socket_entrenador);

	int tamanio_coord = sizeof(t_posicion);
	void * coordenadas = malloc(tamanio_coord);

	if(recv(socket_entrenador, coordenadas, tamanio_coord, 0) < 0) {
		perror("Error en el recv de _UBICACION_POKENEST");
		pokenestLocalizada = false;
	}

	memcpy(&(ubicacionProximaPokenest->x), coordenadas, 4);
	memcpy(&(ubicacionProximaPokenest->y), coordenadas + 4, 4);

	if ((ubicacionProximaPokenest->x == -1)||(ubicacionProximaPokenest->y == -1)) {
		perror("Error en las coordenadas recibidas");
		pokenestLocalizada = false;
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

	recibir_header(socket_entrenador);
}

void enviarUbicacionAMapa(){

	enviar_header(_MOVER_XY, sizeof(t_posicion), socket_entrenador);

	void* buffer_out = malloc(sizeof(t_posicion));
	memcpy(buffer_out,&(ubicacionActual->x),4);
	memcpy(buffer_out + 4,&(ubicacionActual->y),4);

	send(socket_entrenador,buffer_out,sizeof(t_posicion),0);
}

void atraparPokemon(){
	t_pokemon * pokemonAtrapado = malloc(sizeof(t_pokemon));

	enviar_header(_CAPTURAR_PKM,0,socket_entrenador);

	//escuchar y hay dos posibilidades, (deadlock y vuelvo a escuchar) o (pokemon y sigo la rutina)

	t_header * header_in = recibir_header(socket_entrenador);
	bool pokemonCapturadoOEntrenadorMuerto = false;

	while (!pokemonCapturadoOEntrenadorMuerto) {
		switch (header_in->identificador) {
		case _CAPTURAR_PKM:

			pokemonAtrapado->nombreArchivo = malloc(header_in->tamanio);
			recv(socket_entrenador, pokemonAtrapado->nombreArchivo, header_in->tamanio,0);

			//copiarPokemon(pokemonAtrapado);

			pokemonCapturadoOEntrenadorMuerto = true;
			list_add(pokemonesCapturados, pokemonAtrapado);

			puts("pkm capturado");

			verificarNivelPokemon(pokemonAtrapado);
			verificarSiQuedanObjetivosEnMapa();
			pokenestLocalizada = false;
			break;

		case _BATALLA:
			pokemonCapturadoOEntrenadorMuerto = batallaPokemon();
			break;

		default:
			puts("identificador de header invalido");
			exit(EXIT_FAILURE);
			break;
		}
	}
}

void verificarSiQuedanObjetivosEnMapa(){
	if(queue_size(mapaActual->objetivos) == 0){
			enviar_header(_OBJETIVO_CUMPLIDO,0,socket_entrenador);

			//t_header * header = recibir_header(socket_entrenador);

			//TODO mapa envia una estructura con los tiempos y la ruta de la medalla
			//TODO sumar tiempos
			//copiarMedalla();
			desconectarseDeMapa();
			queue_pop(metadata->viaje);

			if(queue_size(metadata->viaje) == 0){
				finDelJuego = true;
			}
			else{
				conectarseConSiguienteMapa();
			}
		}
	else{

		enviar_header(_QUEDAN_OBJETIVOS,0,socket_entrenador);
	}
}

void verificarNivelPokemon(t_pokemon * pokemon){

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

	puts("******--------******");
	puts("Te has convertido en un maestro pokemon!");
	printf("El tiempo total de tu aventura fue de: %f segundos \n ", tiempoDeJuego);
	printf("Pasaste %f segundos bloqueado \n", tiempoBloqueado);
	printf("Estuviste involucrado en %d deadlocks \n", deadlocksInvolucrados);
	printf("Has muerto %d veces \n", cantidadDeMuertes);

}

bool batallaPokemon(){ 					//retorna true si muere

	enviar_header(_PKM_MAS_FUERTE, sizeof(t_pokemon), socket_entrenador);

	enviarPokemon(pokemonMasFuerte,  socket_entrenador);


	t_header * header = recibir_header(socket_entrenador); //mapa envia si entrenador gana la batalla o no

	if (header->identificador != _RESULTADO_BATALLA) {

		puts("identificador de header desconocido");

	} else {

		if (header->tamanio == 0) {
			puts("El entrenador ha perdido una batalla pokemon");
			muereEntrenador = true;
			return true;
		}
	}
	return false;
}

void muerteEntrenador(){
	muereEntrenador = false;
	cantidadDeMuertes += 1;
	puts("El entrenador a muerto");

	if(metadata->vidas > 0){
		metadata->vidas -= 1;
		desconectarseDeMapa();
		//Se reconecta al mismo mapa
		conectarseConSiguienteMapa();
	}
	else{
		char  respuesta;
		bool contestaConOtroCaracter = true;

		printf("No quedan vidas disponibles, cantidad de reintentos: %d \n", metadata->reintentos);
		puts("¿Desea reiniciar el juego?");
		puts("1/0");

		while(contestaConOtroCaracter){
		scanf(respuesta);

		switch(respuesta){

		case 1:
			metadata->reintentos += 1;
			//todo borrar medallas
			desconectarseDeMapa();
			finalizarEntrenador();
			inicializarSinmuertesNiReintentos();
			conectarseConSiguienteMapa();
			contestaConOtroCaracter = false;
			break;

		case 0:
			contestaConOtroCaracter = false;
			exit(EXIT_SUCCESS);
			break;

		default:
			puts("conteste con 1/0");
		}
		}
	}

}

void desconectarseDeMapa(){

	close(socket_entrenador);  //cierra la conexion?
	//TODO borrar pokemones de directorio
	list_clean(pokemonesCapturados);
	pokenestLocalizada = false;

}

void finalizarEntrenador(){
	free(metadata->nombre);
	free(ubicacionActual);
	queue_destroy_and_destroy_elements(metadata->viaje, (void*) destruirHojaDeViaje);
	free(ubicacionProximaPokenest);
	list_destroy(pokemonesCapturados);
	queue_destroy(mapaActual->objetivos);
	free(mapaActual);
	free(pokemonMasFuerte);

}

void destruirHojaDeViaje(t_mapa * mapa){
	free(mapa->nombre_mapa);
	queue_destroy(mapa->objetivos);
}

void enviarPokemon(t_pokemon * pokemon, int socket){

	int pokemonSerializadoSize = (sizeof(pokemon->nivel)) + (string_length(pokemon->nombre)) + (string_length(pokemon->nombreArchivo)) + (2 * (sizeof(int)));

	void * pokemonSerializado = serializarPokemon(pokemonMasFuerte);

	enviar_header(_PKM_MAS_FUERTE, pokemonSerializadoSize,socket);

	send(socket,pokemonSerializado,pokemonSerializadoSize,0);

}

void * serializarPokemon(t_pokemon * pokemon){

	int nombreSize = (string_length(pokemon->nombre));
	int nombreArchivoSize = (string_length(pokemon->nombreArchivo));

	int pokemonSerializadoSize = (sizeof(pokemon->nivel)) + nombreSize + nombreArchivoSize + (2 * (sizeof(int)));

	void* pokemonSerializado = malloc(pokemonSerializadoSize);


	memcpy(pokemonSerializado,&nombreSize,sizeof(int));
	memcpy(pokemonSerializado + 4,&nombreArchivoSize , sizeof(int));
	memcpy(pokemonSerializado + 8,&(pokemon->nivel) , sizeof(int));
	memcpy(pokemonSerializado + 12, pokemon->nombre, nombreSize);
	memcpy(pokemonSerializado + 12 + nombreSize, pokemon->nombreArchivo, nombreArchivoSize);

	return pokemonSerializado;
}

//TODO Reemplazar por funcion de juli

/*
void copiarPokemon(t_pokemon * pokemonAtrapado){

	char* comando = malloc(sizeof(char) * 100);
	char* rutaDirDeBill = string_duplicate(metadata_path);

	string_append(&rutaDirDeBill,"Entrenadores/");
	string_append(&rutaDirDeBill,nombreEntrenador);
	string_append(&rutaDirDeBill,"/Dir de Bill");

	sprintf(comando, "cp %s %s", pokemonAtrapado->nombreArchivo ,rutaDirDeBill );

	system(comando);

	//TODO falta copiar el nombre (talvez hacer un define con los char identificadores)
	t_config * conf_file = config_create(pokemonAtrapado->nombreArchivo);

	pokemonAtrapado->nivel = getIntProperty(conf_file, "Nivel");
	free(conf_file);
	free(comando);

}

void copiarMedalla(){

	char* comando = malloc(sizeof(char) * 100);
	char* rutaDirDeMedalla = string_duplicate(metadata_path);
	char* rutaDirDeBill = string_duplicate(metadata_path);

	string_append(&rutaDirDeMedalla,"Mapas/");
	string_append(&rutaDirDeMedalla,mapaActual->nombre_mapa);
	string_append(&rutaDirDeMedalla,"/medalla-[");
	string_append(&rutaDirDeMedalla,mapaActual->nombre_mapa);
	string_append(&rutaDirDeMedalla,"].jpg");


	string_append(&rutaDirDeBill,"Entrenadores/");
	string_append(&rutaDirDeBill,nombreEntrenador);
	string_append(&rutaDirDeBill,"/Dir de Bill");

	sprintf(comando, "cp %s %s", rutaDirDeMedalla , rutaDirDeBill );

	system(comando);

	free(comando);
	free(rutaDirDeBill);
	free(rutaDirDeMedalla);
}
*/
