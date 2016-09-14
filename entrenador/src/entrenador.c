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

int leer_metadata_mapa(char * metada_path) {							//TODO Hay que cambiarlo para que busque la ruta exacta
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
			break;

		case SIGTERM:
			metadata->vidas -= 1;
			break;

		default: puts("Codigo de señal invalida");
	}
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
	ubicacionProximaPokenest = NULL;
	pokemonesCapturados = list_create();
	pokemonMasFuerte = NULL;
	finDelJuego = false;
	ubicacionActual = malloc(sizeof(t_posicion));
}
void cargarMetadata(){

	leer_metadata_mapa(metadata_path);
	imprimir_metadata();
}

void conectarseConSiguienteMapa(){

	mapaActual = queue_peek(metadata->viaje);
//
//	socket_entrenador = conectarse_a_un_mapa(mapaActual->puerto,
//			mapaActual->ip);

	socket_entrenador = conectarse_a_un_mapa(9000, "127.0.0.1");

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

void solicitarUbicacionDelProximoPokenest(){
	char id_pokemon;							//talvez hay que usar char*

	id_pokemon = queue_pop(mapaActual->objetivos);

	enviarSolicitudUbicacionPokenest(id_pokemon);

}

void enviarSolicitudUbicacionPokenest(char id_pokemon){
	//TODO enviar a mapa

	//TODO asignar ubicaciones a ubicacionProximaPokenest
	pokenestLocalizada = true;
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

	//TODO informar movimiento al mapa
}

void atraparPokemon(){
		t_pokemon * pokemonAtrapado;

	//TODO enviar a mapa que quiero atrapar

	//escuchar y hay dos posibilidades, (deadlock y vuelvo a escuchar) o (pokemon y sigo la rutina)

	//TODO podria recibir que hay un deadlock => batalla y return


	//Todo copiar archivo de pokemon y asignarlo a pokemonAtrapado

	list_add(pokemonesCapturados, pokemonAtrapado);

	verificarNivelPokemon(pokemonAtrapado);
	verificarSiQuedanObjetivosEnMapa();

	pokenestLocalizada = false;

}

void verificarSiQuedanObjetivosEnMapa(){
	if(queue_size(mapaActual->objetivos) == 0){
			//TODO avisar a mapa que se cumplio objetivo
			//TODO copiar medalla
			desconectarseDeMapa();
			queue_pop(metadata->viaje);

			if(queue_size(metadata->viaje) == 0){
				finDelJuego = true;
			}
			else{
				conectarseConSiguienteMapa();
			}
			//TODO informar que ya termine el mapa
		}
	else{
		//TODO informar a mapa que tengo mas objetivos
	}
}

void verificarNivelPokemon(t_pokemon * pokemon){
	if(pokemon->nivel > pokemonMasFuerte->nivel){

		pokemonMasFuerte = pokemon;

	}
}

void esperarTurno(){

	// esperar a que mapa ceda el turno
	t_header * head_in = recibir_header(socket_entrenador);

	if (head_in == NULL) {
		perror("error en la recepcion del turno");
		exit(EXIT_FAILURE);
	}

	if (head_in->identificador != _TURNO_CONCEDIDO) {
		perror("error en el id del turno concedido");
		exit(EXIT_FAILURE);
	}


}

void realizarAccion(){

	if(!pokenestLocalizada){
		solicitarUbicacionDelProximoPokenest();
		return;
	}

	if(estoyEnPokenest()){
		atraparPokemon();
		return;
	}

	if(!estoyEnPokenest()){
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
	//TODO imprimir por pantalla el logro
}

void batallaPokemon(){
	//TODO enviar a mapa pokemonMasFuerte
	//TODO recibo si vivo o muero

	muerteEntrenador();
}

void muerteEntrenador(){
	cantidadDeMuertes += 1;
	puts("El entrenador a muerto debido a que perdio una batalla pokemon");

	if(metadata->vidas > 0){
		metadata->vidas -= 1;
		desconectarseDeMapa();
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

	//TODO desconectarse de  mapa
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

