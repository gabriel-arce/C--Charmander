/*
 * shared_serializacion.c
 *
 *  Created on: 11/8/2016
 *      Author: utnso
 */

#include "shared_serializacion.h"

t_header * crear_header(int id, int size) {
	t_header * header = malloc(sizeof(t_header));

	if (header == NULL)
		return NULL;

	header->identificador = (uint8_t) id;
	header->tamanio = (uint32_t) size;

	return header;
}

void * serializar_header(int id, int size) {
	t_header * header = crear_header(id, size);

	void * buffer = malloc(5);
	memcpy(buffer, &(header->identificador), 1);
	memcpy(buffer + 1, &(header->tamanio), 4);

	free(header);

	return buffer;
}

t_header * deserializar_header(void * buffer) {
	t_header * header = malloc(sizeof(t_header));

	memcpy(&(header->identificador), buffer, 1);
	memcpy(&(header->tamanio), buffer + 1, 4);

	free(buffer);

	return header;
}
