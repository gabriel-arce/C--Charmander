/*
 * shared_comunicaciones.c
 *
 *  Created on: 12/8/2016
 *      Author: utnso
 */

#include "shared_comunicaciones.h"

//----------------------------------------------------->

int enviar_header(int id, int tamanio, int socket) {
	void * buffer = serializar_header((uint8_t) id, (uint32_t) tamanio);

	int result = send(socket, buffer, 5, 0);

	if (result <= 0) {
		//printf("Error en el envio del header %d\n", id);
	}

	free(buffer);
	return result;
}

t_header * recibir_header(int socket) {
	void * buffer = malloc(5);

	int result = recv(socket, buffer, 5, 0);

	if (result <= 0) {
		//puts("Error en el recv del header");
		return NULL;
	}

	t_header * header = deserializar_header(buffer);

	return header;
}

//----------------------------------------------------->

int enviar_handshake(int socket, int id) {
	void * buffer = serializar_header(id, 0);

	int result = send(socket, buffer, 5, 0);

	if (result == -1) {
		puts("\nError en el envio del handshake\n");
	} else {
		puts("\nHandshake enviado\n");
	}

	free(buffer);
	return result;
}

int recibir_handshake(int socket) {
	void * buffer = malloc(5);
	int handshake_id = 0;

	int result = recv(socket, buffer, 5, 0);

	if (result <= 0) {
		puts("Error en el recv del handshake");
		return -1;
	}

	t_header * header = deserializar_header(buffer);

	handshake_id = (int) header->identificador;
	free(header);

	return handshake_id;
}
//----------------------------------------------------->
