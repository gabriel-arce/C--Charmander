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

void * serializarPokemon(t_pkm * pokemon){

	int nombreSize = (string_length(pokemon->nombre));
	int nombreArchivoSize = (string_length(pokemon->nombreArchivo));

	int pokemonSerializadoSize = nombreSize + nombreArchivoSize + (2 * (sizeof(int))) + sizeof(bool) + sizeof(char);

	void* pokemonSerializado = malloc(pokemonSerializadoSize);


	memcpy(pokemonSerializado,&nombreSize,sizeof(int));
	memcpy(pokemonSerializado + 4,&nombreArchivoSize , sizeof(int));
	memcpy(pokemonSerializado + 8,&(pokemon->nivel) , sizeof(int));
	memcpy(pokemonSerializado + 12,&(pokemon->mapa) , sizeof(int));
	memcpy(pokemonSerializado + 16, pokemon->nombre, nombreSize);
	memcpy(pokemonSerializado + 16 + nombreSize, pokemon->nombreArchivo, nombreArchivoSize);
	memcpy(pokemonSerializado + 16 + nombreSize + nombreArchivoSize, pokemon->capturado, sizeof(bool));

	return pokemonSerializado;

}

t_pkm * deserializarPokemon(void* pokemonSerializado){
	t_pkm * pokemon = malloc(sizeof(t_pkm));
	int nombreSize;
	int nombreArchivoSize;

	memcpy(&nombreSize,pokemonSerializado,sizeof(int));
	memcpy(&nombreArchivoSize,pokemonSerializado + sizeof(int),sizeof(int));

	memcpy(&pokemon->nivel,pokemonSerializado + 2*(sizeof(int)), sizeof(int));
	memcpy(pokemon->nombre, pokemonSerializado + 3*(sizeof(int)), nombreSize);
	memcpy(pokemon->nombreArchivo,pokemonSerializado + 3*(sizeof(int)) + nombreSize, nombreArchivoSize);

	return pokemon;
}


int enviarPorSocket(int fdCliente, const void * mensaje, int tamanioBytes)
{
	int bytes_enviados = 0;
	int totalBytes = 0;

	while (totalBytes < tamanioBytes)
	{
		bytes_enviados = send(fdCliente, mensaje + totalBytes, tamanioBytes, MSG_NOSIGNAL);
/* send: devuelve el múmero de bytes que se enviaron en realidad, pero como estos podrían ser menos
* de los que pedimos que se enviaran, realizamos la siguiente validación: */

		if (bytes_enviados == ERROR)
		{
			break;
		}
		totalBytes += bytes_enviados;
		tamanioBytes -= bytes_enviados;
	}
	if (bytes_enviados == ERROR)
	{
		printf(RED "\n\t Error al enviar los datos\n" RESET);
	}
	return bytes_enviados; // En caso de éxito, se retorna la cantidad de bytes realmente enviada
}

// Recibir algo a través de sockets
int recibirPorSocket(int skServidor, void * buffer, int tamanioBytes)
{
	int total = 0;
	int bytes_recibidos = 0;

	while (total < tamanioBytes)
	{

		bytes_recibidos = recv(skServidor, buffer+total, tamanioBytes, MSG_WAITALL);
		// MSG_WAITALL: el recv queda completamente bloqueado hasta que el paquete sea recibido completamente

		if (bytes_recibidos <  0)
		{ // Error al recibir mensaje
			printf(RED "\n\t No se recibieron correctamente los datos.\n" RESET);
			return -1;
		}

		if (bytes_recibidos == 0)
		{
	   	printf(RED "\n\t La conexión socket #%d se ha cerrado.\n" RESET, skServidor);
			return -1;
		}
		total += bytes_recibidos;
		tamanioBytes -= bytes_recibidos;
	}
	return bytes_recibidos; // En caso de éxito, se retorna la cantidad de bytes realmente recibida
}

int calcularTamanioMensaje(int head, void* mensaje)
{
	int tamanio = 0;
	switch(head)
	{
		case HANDSHAKE:
			tamanio = strlen((char*) mensaje) + 1;
			break;

		case PEDIDO_GETATTR:
			tamanio = strlen((char*) mensaje) + 1;
			break;

		case RESPUESTA_GETATTR:
			tamanio = sizeof(t_stbuf);
			break;

		case PEDIDO_READDIR:
			tamanio = strlen((char*) mensaje) + 1;
			break;

		case RESPUESTA_READDIR:
			tamanio =  strlen((char*) mensaje) + 1;
			break;

		case PEDIDO_READ:
			tamanio = sizeof(t_readbuf);
			break;

		case RESPUESTA_READ:
			tamanio = strlen((char*) mensaje) + 1;
			break;

		case PEDIDO_WRITE:
			break;
		case PEDIDO_UNLINK:
			break;
		case PEDIDO_MKDIR:
			break;
		case PEDIDO_RMDIR:
			break;
		case PEDIDO_RENAME:
			break;
		case PEDIDO_CREATE:
			break;
		case RESPUESTA_WRITE:
			break;
		case RESPUESTA_UNLINK:
			break;
		case RESPUESTA_MKDIR:
			break;
		case RESPUESTA_RMDIR:
			break;
		case RESPUESTA_RENAME:
			break;
		case RESPUESTA_CREATE:
			break;

		case ENOENTRY:
			tamanio = sizeof(int);
			break;

		default:
			break;
}
	return tamanio;
}

void* serializarPedidoGetatrr(t_stbuf* response, int tamanio)
{
	int desplazamiento = 0;

	// Copio los campos enteros:
	void* buffer = malloc(tamanio);
	memset(buffer, 0, tamanio);

	memcpy(buffer + desplazamiento, &(response->mode), sizeof(mode_t));
	desplazamiento += sizeof(mode_t);

	memcpy(buffer + desplazamiento, &(response->nlink), sizeof(nlink_t));
	desplazamiento += sizeof(nlink_t);

	memcpy(buffer + desplazamiento, &(response->size), sizeof(off_t));


	// memcpy(buffer + desplazamiento, &(response->st_dev), sizeof(dev_t));
	// desplazamiento += sizeof(dev_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_ino), sizeof(ino_t));
	// desplazamiento += sizeof(ino_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_mode), sizeof(mode_t));
	// desplazamiento += sizeof(mode_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_nlink), sizeof(nlink_t));
	// desplazamiento += sizeof(nlink_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_uid), sizeof(uid_t ));
	// desplazamiento += sizeof(uid_t );
	//
	// memcpy(buffer + desplazamiento, &(response->st_gid), sizeof(gid_t));
	// desplazamiento += sizeof(gid_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_rdev), sizeof(dev_t));
	// desplazamiento += sizeof(dev_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_size), sizeof(off_t));
	// desplazamiento += sizeof(off_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_blksize), sizeof(blksize_t));
	// desplazamiento += sizeof(blksize_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_blocks), sizeof(blkcnt_t));
	// desplazamiento += sizeof(blkcnt_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_atime), sizeof(time_t));
	// desplazamiento += sizeof(time_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_mtime), sizeof(time_t));
	// desplazamiento += sizeof(time_t);
	//
	// memcpy(buffer + desplazamiento, &(response->st_ctime), sizeof(time_t));
	// desplazamiento += sizeof(time_t);

	return buffer;
}

// SERIALIZAR: Del mensaje listo para enviar, al buffer
void* serializar(int head, void* mensaje, int tamanio)
{
	void * buffer = NULL;

	switch(head)
	{
		case HANDSHAKE:
			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;

		case PEDIDO_GETATTR://quiero enviar un path
			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;

		case RESPUESTA_GETATTR://quiero enviar un struct stat
			buffer = serializarPedidoGetatrr((t_stbuf*) mensaje, tamanio);
			break;

		case PEDIDO_READDIR:// envio un path
			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;

		case RESPUESTA_READDIR:// envio un char*
			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;

		case PEDIDO_READ:
			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;

		case RESPUESTA_READ:
			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;

		case PEDIDO_WRITE:
			break;
		case PEDIDO_UNLINK:
			break;
		case PEDIDO_MKDIR:
			break;
		case PEDIDO_RMDIR:
			break;
		case PEDIDO_RENAME:
			break;
		case PEDIDO_CREATE:
			break;
		case RESPUESTA_WRITE:
			break;
		case RESPUESTA_UNLINK:
			break;
		case RESPUESTA_MKDIR:
			break;
		case RESPUESTA_RMDIR:
			break;
		case RESPUESTA_RENAME:
			break;
		case RESPUESTA_CREATE:
			break;

		case ENOENTRY:
			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;

		default:
			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;
		}

	return buffer;
}

// DESERIALIZAR: Del buffer, al mensaje listo para recibir
void * deserializar(int head, void * buffer, int tamanio)
{
	void * mensaje = NULL;

	switch(head)
	{
		case HANDSHAKE:
			mensaje = malloc(tamanio);
			memcpy(mensaje, buffer, tamanio);
			break;

		case PEDIDO_GETATTR:
			mensaje = malloc(tamanio);
			memcpy(mensaje, buffer, tamanio);
			break;

		case RESPUESTA_GETATTR:
			mensaje = serializarPedidoGetatrr((t_stbuf*)buffer, tamanio);
			break;

		case PEDIDO_READDIR:
			mensaje = malloc(tamanio);
			memcpy(mensaje, buffer, tamanio);
			break;

		case RESPUESTA_READDIR:
			mensaje = malloc(tamanio);
			memcpy(mensaje, buffer, tamanio);
			break;

		case PEDIDO_READ:
			mensaje = malloc(tamanio);
			memcpy(mensaje, buffer, tamanio);
			break;

		case RESPUESTA_READ:
			mensaje = malloc(tamanio);
			memcpy(mensaje, buffer, tamanio);
			break;

		case PEDIDO_WRITE:
			break;
		case PEDIDO_UNLINK:
			break;
		case PEDIDO_MKDIR:
			break;
		case PEDIDO_RMDIR:
			break;
		case PEDIDO_RENAME:
			break;
		case PEDIDO_CREATE:
			break;
		case RESPUESTA_WRITE:
			break;
		case RESPUESTA_UNLINK:
			break;
		case RESPUESTA_MKDIR:
			break;
		case RESPUESTA_RMDIR:
			break;
		case RESPUESTA_RENAME:
			break;
		case RESPUESTA_CREATE:
			break;

		case ENOENTRY:
			mensaje = malloc(tamanio);
			memcpy(mensaje, buffer, tamanio);
			break;

		default:
			mensaje = malloc(tamanio);
			memcpy(mensaje, buffer, tamanio);
			break;
	}
	return mensaje;
} // Se debe castear lo retornado (indicar el tipo de dato que debe matchear con el void*)

int enviarConProtocolo(int fdReceptor, int head, void *mensaje)
{
	int desplazamiento = 0, tamanioMensaje = 0, tamanioTotalAEnviar = 0;

	tamanioMensaje = calcularTamanioMensaje(head, mensaje);

	void *mensajeSerializado = serializar(head, mensaje, tamanioMensaje);

	// Lo que se envía es: head + tamaño del msj + el msj serializado:
	tamanioTotalAEnviar = 2* sizeof(int) + tamanioMensaje;

	// Meto en el buffer las tres cosas:
	void *buffer = malloc(tamanioTotalAEnviar);
	memcpy(buffer + desplazamiento, &head, sizeof(int));
	desplazamiento +=  sizeof(int);
	memcpy(buffer + desplazamiento, &tamanioMensaje,  sizeof(int));
	desplazamiento +=  sizeof(int);
	memcpy(buffer + desplazamiento, mensajeSerializado, tamanioMensaje);

	// Envío la totalidad del paquete (lo contenido en el buffer):
	int enviados = enviarPorSocket(fdReceptor, buffer, tamanioTotalAEnviar);

	free(mensajeSerializado);
	mensajeSerializado = NULL;
	free(buffer);
	buffer = NULL;

	return enviados;
}

void* recibirConProtocolo(int socketEmisor, int* head)
{   // Validar contra NULL al recibir en cada módulo.

	// Recibo primero el head:
	int recibido = recibirPorSocket(socketEmisor, head, sizeof(int));

	if (*head < 1 || recibido <= 0)
	{
		return NULL;
	}

	// Recibo ahora el tamaño del mensaje:
	int* tamanioMensaje = malloc(sizeof(int));
	recibido = recibirPorSocket(socketEmisor, tamanioMensaje, sizeof(int));

	if (recibido <= 0)
	{
		return NULL;
	}

	// Recibo por último el mensaje serializado:
	void* mensaje = malloc(*tamanioMensaje);
	recibido = recibirPorSocket(socketEmisor, mensaje, *tamanioMensaje);

	if (recibido <= 0)
	{
		return NULL;
  }

	// Deserializo el mensaje según el protocolo:
	void* buffer = deserializar(*head, mensaje, *tamanioMensaje);

	free(tamanioMensaje); tamanioMensaje = NULL;
	free(mensaje); mensaje = NULL;

	return buffer;
} // Se debe castear el mensaje al recibirse (indicar el tipo de dato que debe matchear con el void*)

void* recibirEstructuraRead(int socketEmisor,int* head)
{
	// Recibo el head: PEDIDO_READ
	printf(MAG"\t En recibirEstructuraRead head:%d\n ", *head);
	int recibido = recibirPorSocket(socketEmisor, head, sizeof(int));
	printf("\t En recibirEstructuraRead recibi algo :%d\n ", recibido);
	if (*head < 1 || recibido <= 0)
	{
		return NULL;
	}

	// Recibo el struct t_readbuf:
	t_readbuf* buffer = malloc(sizeof(t_readbuf));
	recibido = recibirPorSocket(socketEmisor, buffer, sizeof(t_readbuf));
	printf("\t En recibirEstructuraRead recibi t_readbuf tamano :%d\n " , recibido);
	if (recibido <= 0)
	{
		return NULL;
	}

	// Recibo  el path:
	char* path = malloc(buffer->pathLen);
	recibido = recibirPorSocket(socketEmisor, path, buffer->pathLen);
	printf("\t En recibirEstructuraRead recibi path :%s\n" RESET, path);
	if (recibido <= 0)
	{
		return NULL;
	}

	void* bufferEntero = serializarPedidoRead(buffer, path);

	free(path);
	free(buffer);
	path = NULL;
	buffer = NULL;
	return bufferEntero;
} // Se debe castear el mensaje al recibirse (indicar el tipo de dato que debe matchear con el void*)

void* serializarPedidoRead(t_readbuf* response, char* path)
{
	int desplazamiento = 0;

	void* buffer = malloc(sizeof(t_readbuf) + response->pathLen);
	memset(buffer, 0, sizeof(t_readbuf) + response->pathLen);

	memcpy(buffer + desplazamiento, &(response->size), sizeof(size_t));
	desplazamiento += sizeof(size_t);
	memcpy(buffer + desplazamiento, &(response->offset), sizeof(off_t));
	desplazamiento += sizeof(off_t);
	memcpy(buffer + desplazamiento, &(response->pathLen), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, path, response->pathLen);//chequear aca por si path necesita & o *
	printf(MAG "\t En serializarPedidoRead el path es:%s\n" RESET, path);
  return buffer;
}

int enviarEstructuraRead(int fdReceptor, int head, char* path, t_readbuf* mensaje )
{
	int desplazamiento = 0;
	int tamanioMensaje = sizeof(t_readbuf) + strlen(path) + 1;
	int tamanioTotalAEnviar = 0;
	printf(MAG "\t En enviarEstructuraRead path:%s\n ",path);
	void *mensajeSerializado = serializarPedidoRead( mensaje, path);

	// Lo que se envía es: head + tamaño del msj serializado:
	tamanioTotalAEnviar = sizeof(int) + tamanioMensaje;

	// Agrego todo junto al buffer para enviar
	void *buffer = malloc(tamanioTotalAEnviar);
	memcpy(buffer + desplazamiento, &head, sizeof(int));
	desplazamiento +=  sizeof(int);
	memcpy(buffer + desplazamiento, mensajeSerializado, tamanioMensaje);

	// Envío la totalidad del paquete (lo contenido en el buffer):
	int enviados = enviarPorSocket(fdReceptor, buffer, tamanioTotalAEnviar);
	printf("\t En enviarEstructuraRead enviados:%d\n" RESET,enviados);

	free(mensajeSerializado);
	mensajeSerializado = NULL;
	free(buffer);
	buffer = NULL;

	return enviados;
}
