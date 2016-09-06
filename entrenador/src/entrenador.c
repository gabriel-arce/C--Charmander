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
