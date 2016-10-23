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

	if (buffer == NULL)
		return NULL;

	t_header * header = malloc(sizeof(t_header));

	memcpy(&(header->identificador), buffer, 1);
	memcpy(&(header->tamanio), buffer + 1, 4);

	free(buffer);

	return header;
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

t_pokemon * deserializarPokemon(void* pokemonSerializado){
	t_pokemon * pokemon = malloc(sizeof(t_pokemon));
	int nombreSize;
	int nombreArchivoSize;

	memcpy(nombreSize,pokemonSerializado,sizeof(int));
	memcpy(nombreArchivoSize,pokemonSerializado + sizeof(int),sizeof(int));

	memcpy(pokemon->nivel,pokemonSerializado + 2*(sizeof(int)), sizeof(int));
	memcpy(pokemon->nombre, pokemonSerializado + 3*(sizeof(int)), nombreSize);
	memcpy(pokemon->nombreArchivo,pokemonSerializado + 3*(sizeof(int)) + nombreSize, nombreArchivoSize);

	return pokemon;
}
