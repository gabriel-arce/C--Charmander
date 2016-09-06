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

	return mapa;
}

t_metadata_entrenador * crear_metadata() {
	t_metadata_entrenador * metadata = malloc(sizeof(t_metadata_entrenador));
	memset(metadata, 0, sizeof(t_metadata_entrenador));

	metadata->viaje = list_create();

	return metadata;
}

int leer_metadata(char * metada_path) {
	metadata = crear_metadata();

	t_config * conf_file = config_create(metada_path);

	metadata->nombre = string_duplicate(getStringProperty(conf_file, "nombre"));

	metadata->simbolo = (char) string_duplicate(getStringProperty(conf_file, "simbolo"))[0];

	t_list * hoja_de_viaje = getListProperty(conf_file, "hojaDeViaje");

	if (hoja_de_viaje == NULL)
		return -1;

	int ciudades = list_size(hoja_de_viaje);
	int i;
	for (i = 0; i < ciudades; i++) {
		char * ciudad = list_get(hoja_de_viaje, i);
		t_mapa * mapa_i = crear_mapa(ciudad);

		char * property_name = string_duplicate("obj[");
		string_append(&(property_name), ciudad);
		string_append(&(property_name), "]");

		mapa_i->objetivos = getListProperty(conf_file, property_name);

		list_add_in_index(metadata->viaje, i, mapa_i);

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

	int ciudades = list_size(metadata->viaje);
	int i;

	for(i = 0; i < ciudades; i++) {
		t_mapa * mapa = list_get(metadata->viaje, i);
		printf("%s\n", mapa->nombre_mapa);
		printf("objetivos: [");
		int objetivos = list_size(mapa->objetivos);
		int j;
		for (j = 0; j < objetivos; j++) {
			printf(" %s ", (char *) list_get(mapa->objetivos, j));
		}
		printf("]\n");
	}

	printf("\n");
}

int conectarse_a_un_mapa(int puerto, char * ip) {
	int socket_fd = -1;

	socket_fd = clienteDelServidor(ip, puerto);

	if (enviar_handshake(socket_fd, 1) == -1)
		return -1;

	return socket_fd;
}

int enviar_datos_a_mapa(int socket, char simbolo, char * nombre) {

	int buffer_size = sizeof(char) + string_length(nombre);
	void * data_buffer = malloc(buffer_size);

	if (send(socket, &(buffer_size), sizeof(int), 0) < 0)
		return -1;

	memcpy(data_buffer, &(simbolo), sizeof(char));
	memcpy(data_buffer + sizeof(char), nombre, string_length(nombre));

	if (send(socket, data_buffer, buffer_size, 0) < 0)
		return -1;

	return EXIT_SUCCESS;
}
