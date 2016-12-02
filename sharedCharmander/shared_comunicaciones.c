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

int serializarYEnviarPokemon(int id_header, t_pkm * pokemon, int socket) {

	int nombreSize = (string_length(pokemon->nombre));
	int nombreArchivoSize = (string_length(pokemon->nombreArchivo));

	//4(nombreSize) + 4(nombreArchivoSize) + 4(nivel) + nombre + nombreArchivo + 1(capturado) + 1(id_pokenest)
	int pokemonSerializadoSize = (3 * sizeof(int)) + nombreSize + nombreArchivoSize + sizeof(bool) + sizeof(char);

	if ( enviar_header(id_header, pokemonSerializadoSize, socket) < 0 )
		return EXIT_FAILURE;

	void* pokemonSerializado = malloc(pokemonSerializadoSize);

	int offset = sizeof(int);
	memcpy(pokemonSerializado, &nombreSize, offset);
	memcpy(pokemonSerializado + offset ,&nombreArchivoSize , offset);
	offset += sizeof(int);
	memcpy(pokemonSerializado + offset, &(pokemon->nivel), sizeof(int));
	offset += sizeof(int);
	memcpy(pokemonSerializado + offset, pokemon->nombre, nombreSize);
	offset += nombreSize;
	memcpy(pokemonSerializado + offset, pokemon->nombreArchivo, nombreArchivoSize);
	offset += nombreArchivoSize;
	memcpy(pokemonSerializado + offset, &(pokemon->capturado), sizeof(bool));
	offset += sizeof(bool);
	memcpy(pokemonSerializado + offset, &(pokemon->id_pokenest), sizeof(char));
	offset += sizeof(char);

	int resultado = EXIT_SUCCESS;

	if ( send(socket, pokemonSerializado, pokemonSerializadoSize, 0) == -1 )
		resultado = EXIT_FAILURE;

	free(pokemonSerializado);

	return resultado;
}

t_pkm * recibirYDeserializarPokemon(int socket, int bytes_recv) {

	void * pokemonSerializado = malloc(bytes_recv);

	if ( recv(socket, pokemonSerializado, bytes_recv, 0) < 0 ) {
		free(pokemonSerializado);
		return NULL;
	}

	t_pkm * pokemon = malloc(sizeof(t_pkm));
	int nombreSize;
	int nombreArchivoSize;
	int offset = sizeof(int);

	memcpy(&nombreSize, pokemonSerializado, offset);
	memcpy(&nombreArchivoSize, pokemonSerializado + offset, offset);
	offset += sizeof(int);

	pokemon->nombre = (char *) malloc(nombreSize + 1);
	pokemon->nombreArchivo = (char *) malloc(nombreArchivoSize + 1);

	memcpy(&(pokemon->nivel), pokemonSerializado + offset, sizeof(int));
	offset += sizeof(int);
	memcpy(pokemon->nombre, pokemonSerializado + offset, nombreSize);
	offset += nombreSize;
	pokemon->nombre[nombreSize] = '\0';
	memcpy(pokemon->nombreArchivo, pokemonSerializado + offset, nombreArchivoSize);
	offset += nombreArchivoSize;
	pokemon->nombreArchivo[nombreArchivoSize] = '\0';
	memcpy(&(pokemon->capturado), pokemonSerializado + offset, sizeof(bool));
	offset += sizeof(bool);
	memcpy(&(pokemon->id_pokenest), pokemonSerializado + offset, sizeof(char));

	free(pokemonSerializado);

	return pokemon;
}
