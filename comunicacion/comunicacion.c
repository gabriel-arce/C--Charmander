

#include "comunicacion.h"


int aceptarConexion(int listenningSocket)
{
	int socketCliente;
	struct sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	if ((socketCliente = accept(listenningSocket, (struct sockaddr *) &addr, &addrlen)) == -1)
			{
		printf(RED "\t Fallo el accept\n" RESET);
		return -1;
	}
//	printf("\t Se conecto el cliente %i\n ",socketCliente);
	return socketCliente;
}

int crearServer(char* puerto)
{
	printf(MAG "\t Puerto donde voy a escuchar conexiones: %s\n" RESET, puerto);

	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	int listenningSocket;
	int yes = 1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;     //IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, puerto, &hints, &servinfo)) != 0)
	{
		printf(RED "\t Fallo el getaddrinfo\n" RESET);
		return -1;
	}

	if ((listenningSocket = socket(servinfo->ai_family, servinfo->ai_socktype,servinfo->ai_protocol)) == -1)
	{
		printf(RED "\t Fallo el socket\n" RESET);
		return -1;
	}
	if (setsockopt(listenningSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		printf(RED "\t Fallo el setsockopt\n" RESET);
		return -1;
	}

	if (bind(listenningSocket, servinfo->ai_addr, servinfo->ai_addrlen))
	{
		printf(RED "\t Fallo el bind\n" RESET);
		return -1;
	}
	freeaddrinfo(servinfo);

	if (listen(listenningSocket, 15))
	{	//Backlog de 15, no creo que sea necesario mas
		printf(RED "\t Fallo el listen\n" RESET);
		return -1;
	}
	printf(MAG "\t Socket servidor creado correctamente\n" RESET);
	return listenningSocket;
}

int crearSocket(char ip[], char puerto[])
{
	struct addrinfo hints;
	struct addrinfo *serverInfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;		// Permite que la maquina se encargue de verificar si usamos IPv4 o IPv6
	hints.ai_socktype = SOCK_STREAM;	// Indica que usaremos el protocolo TCP

	getaddrinfo(ip, puerto, &hints, &serverInfo);	// Carga en serverInfo los datos de la conexion

	int serverSocket;
	if ((serverSocket = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol)) == -1)
	{
		printf(RED "\n\t Error en la llamada a socket()\n" RESET);
		return -1;
	}
	if ((connect(serverSocket, serverInfo->ai_addr, serverInfo->ai_addrlen)) == -1)
	{
		printf(RED "\n\t Error en la llamada a connect()\n" RESET);
		return -2;
	}
	freeaddrinfo(serverInfo);	// No lo necesitamos mas
	printf(MAG "\t Socket creado correctamente\n" RESET);
	return serverSocket;
}

int enviarPorSocket(int fdCliente, void* mensaje, int tamanioBytes)
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
		case PEDIDO_CREATE:
		case PEDIDO_FLUSH:
		case PEDIDO_GETATTR:
		case PEDIDO_MKDIR:
		case PEDIDO_MKNOD:
		case PEDIDO_OPEN:
		case PEDIDO_READDIR:
		case RESPUESTA_READDIR:
		case PEDIDO_RELEASE:
		case PEDIDO_RENAME:
		case PEDIDO_RMDIR:
		case PEDIDO_TRUNCATE:
		case PEDIDO_UNLINK:
		case PEDIDO_UTIMENS:
			tamanio = strlen((char*) mensaje) + 1;
			break;

		case PEDIDO_TRUNCATE_NEW_SIZE:
			tamanio = sizeof(off_t);
			break;

		case RESPUESTA_READ:
			//printf(YEL "\t Entre en RESPUESTA_READ en calcularTamanioMensaje(), devuelvo un sizeof(int)\n" RESET);
			tamanio = sizeof(int);
			break;

		case RESPUESTA_GETATTR:
			tamanio = sizeof(t_stbuf);
			break;

		case PEDIDO_READ:
			tamanio = sizeof(t_readbuf);
			break;

		case PEDIDO_WRITE:
			tamanio = sizeof(t_writebuf);
			break;

		case ERRDQUOT:
		case ERRFBIG:
		case ERRNAMETOOLONG:
		case ERROR:
		case RESPUESTA_CREATE:
		case RESPUESTA_ERROR:
		case RESPUESTA_FLUSH:
		case RESPUESTA_MKDIR:
		case RESPUESTA_MKNOD:
		case RESPUESTA_OPEN:
		case RESPUESTA_RMDIR:
		case RESPUESTA_RENAME:
		case RESPUESTA_UNLINK:
		case RESPUESTA_RELEASE:
		case RESPUESTA_TRUNCATE:
		case RESPUESTA_UTIMENS:

			tamanio = sizeof(char);
			break;

		case RESPUESTA_WRITE:
			tamanio = sizeof(uint32_t);
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
		case ENOENTRY:
		case ERRDQUOT:
		case ERRFBIG:
		case ERRNAMETOOLONG:
		case ERROR:
		case RESPUESTA_ERROR:

		case HANDSHAKE:

		case PEDIDO_CREATE:
		case PEDIDO_GETATTR:
				case PEDIDO_FLUSH:
		case PEDIDO_MKDIR:
		case PEDIDO_OPEN:
		case PEDIDO_READ:
		case PEDIDO_READDIR:
		case PEDIDO_RMDIR:
		case PEDIDO_RENAME:
		case PEDIDO_TRUNCATE:
		case PEDIDO_TRUNCATE_NEW_SIZE:
		case PEDIDO_UNLINK:
		case PEDIDO_UTIMENS:
		case PEDIDO_WRITE:

		case RESPUESTA_CREATE:
		case RESPUESTA_FLUSH:
		case RESPUESTA_MKDIR:
		case RESPUESTA_OPEN:
		case RESPUESTA_READ:
		case RESPUESTA_READDIR:
		case RESPUESTA_RMDIR:
		case RESPUESTA_RENAME:
		case RESPUESTA_TRUNCATE:
		case RESPUESTA_UNLINK:
		case RESPUESTA_UTIMENS:
		case RESPUESTA_WRITE:

			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;

		case RESPUESTA_GETATTR://quiero enviar un struct stat
				buffer = serializarPedidoGetatrr((t_stbuf*) mensaje, tamanio);
				break;

		default:
			buffer = malloc(tamanio);
			memcpy(buffer, mensaje, tamanio);
			break;
		}

	return buffer;
}

int enviar(int fdReceptor, int head, void *mensaje)
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

void* recibir(int socketEmisor, int* head)
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
	void* buffer = serializar(*head, mensaje, *tamanioMensaje);

	free(tamanioMensaje); tamanioMensaje = NULL;
	free(mensaje); mensaje = NULL;

	return buffer;
} // Se debe castear el mensaje al recibirse (indicar el tipo de dato que debe matchear con el void*)


//Exclusivo para read-----------------------------------------------------------------------
void* recibirEstructuraRead(int socketEmisor,int* head)
{
	// Recibo el head: PEDIDO_READ
	int recibido = recibirPorSocket(socketEmisor, head, sizeof(int));

	if (*head < 1 || recibido <= 0)
	{
		return NULL;
	}

	// Recibo el struct t_readbuf:
	t_readbuf* buffer = malloc(sizeof(t_readbuf));
	recibido = recibirPorSocket(socketEmisor, buffer, sizeof(t_readbuf));
	//printf("\t En recibirEstructuraRead recibi t_readbuf tamano :%d\n " , recibido);
	if (recibido <= 0)
	{
		return NULL;
	}

	// Recibo  el path:
	char* path = malloc(buffer->pathLen);
	recibido = recibirPorSocket(socketEmisor, path, buffer->pathLen);
	//printf("\t En recibirEstructuraRead recibi path :%s\n" RESET, path);
	if (recibido <= 0)
	{
		return NULL;
	}

	void* bufferEntero = serializarPedidoRead(buffer, path);

	// free(path);
	// free(buffer);
	// path = NULL;
	// buffer = NULL;
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
	//printf(MAG "\t En serializarPedidoRead el path es:%s\n" RESET, path);
	free(response);
	free(path);
	return buffer;
}

int enviarEstructuraRead(int fdReceptor, int head, char* path, t_readbuf* mensaje)
{
	int desplazamiento = 0;
	int tamanioMensaje = sizeof(t_readbuf) + strlen(path) + 1;
	int tamanioTotalAEnviar = 0;
	//printf(MAG "\t En enviarEstructuraRead path:%s\n ",path);
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
	//printf("\t En enviarEstructuraRead enviados:%d\n" RESET,enviados);

	free(mensajeSerializado);
	mensajeSerializado = NULL;
	free(buffer);
	buffer = NULL;

	return enviados;
}

//Para respuesta read---------------------------------------
int enviarRespuestaRead(int socket, int head, void* respuesta, uint32_t* tamanioBuffer)
{
	int desplazamiento = 0;
	int tamanioAEnviar =sizeof(int) + sizeof(uint32_t) + *tamanioBuffer;//tamanioBuffer podria ser el file_size de un archivo o un size menor si el archivo fuera muy grande

	//Serializo todo junto en el buffer para enviar
	void *buffer = malloc(tamanioAEnviar);
	memcpy(buffer, &head, sizeof(int));
	desplazamiento +=  sizeof(int);
	memcpy(buffer + desplazamiento, tamanioBuffer, sizeof(uint32_t));
	desplazamiento +=  sizeof(uint32_t);
	memcpy(buffer + desplazamiento, respuesta, *tamanioBuffer);

	// Envío la totalidad del paquete de una:
	int enviados = enviarPorSocket(socket, buffer, tamanioAEnviar);
	//printf(MAG "\t En enviarRespuestaRead enviados:%d\n" RESET,enviados);

	free(buffer);
	buffer = NULL;

	return enviados;
}

void* recibirRespuestaRead(int socketEmisor, int* head, uint32_t* tamanio)
{
	// Recibo el head: RESPUESTA_READ
	int recibido = recibirPorSocket(socketEmisor, head, sizeof(int));

	if (*head < 1 || recibido <= 0)
	{
		return NULL;
	}

	//uint32_t tamanio;
	recibido = recibirPorSocket(socketEmisor, tamanio, sizeof(uint32_t));

	if (recibido <= 0)
	{
		return NULL;
	}

	//printf(YEL "\t En recibirRespuestaRead recibi tamanio :%d\n "RESET, *tamanio);
	// Recibo  el buffer:
	void* buffer = malloc(*tamanio);
	recibido = recibirPorSocket(socketEmisor, buffer, *tamanio);

	if (recibido <= 0)
	{
		return NULL;
	}

	return buffer;//bufferEntero;
}

//Exclusivo para write----------------------------------------------------------------------------
void* recibirEstructuraWrite(int socketEmisor,int* head)
{
	// Recibo el head: PEDIDO_WRITE
	//printf(MAG"\t En recibirEstructuraWrite head:%d\n ", *head);
	int recibido = recibirPorSocket(socketEmisor, head, sizeof(int));
	//printf("\t En recibirEstructuraWrite recibi algo :%d\n ", recibido);
	if (*head < 1 || recibido <= 0)
	{
		return NULL;
	}

	// Recibo el struct t_writebuf:
	t_writebuf* buffer = malloc(sizeof(t_writebuf));
	recibido = recibirPorSocket(socketEmisor, buffer, sizeof(t_writebuf));
	//printf("\t En recibirEstructuraWrite recibi t_writebuf tamano :%d\n " , recibido);
	if (recibido <= 0)
	{
		return NULL;
	}

	// Recibo  el path:
	char* path = malloc(buffer->pathLen);
	recibido = recibirPorSocket(socketEmisor, path, buffer->pathLen);
	//printf("\t En recibirEstructuraWrite recibi path :%s\n" RESET, path);
	if (recibido <= 0)
	{
		return NULL;
	}

	//Recibo el buffer con los datos a escribir:
	char* bufWrite = malloc(buffer->bufLen);
	recibido = recibirPorSocket(socketEmisor, bufWrite, buffer->bufLen);
	//printf("\t En recibirEstructuraWrite recibi el buffer para escritura \n" RESET);
	if (recibido <= 0)
	{
		return NULL;
	}

	void* bufferEntero = serializarPedidoWrite(buffer, path, bufWrite);

	free(path);
	free(bufWrite);
	free(buffer);
	path = NULL;
	buffer = NULL;
	bufWrite = NULL;
	return bufferEntero;
} // Se debe castear el mensaje al recibirse (indicar el tipo de dato que debe matchear con el void*)

void* serializarPedidoWrite(t_writebuf* response, char* path, char* bufWrite)
{
	int desplazamiento = 0;

	void* buffer = malloc(sizeof(t_writebuf) + response->pathLen + response->bufLen);
	memset(buffer, 0, sizeof(t_writebuf) + response->pathLen + response->bufLen);

	memcpy(buffer + desplazamiento, &(response->size), sizeof(size_t));
	desplazamiento += sizeof(size_t);
	memcpy(buffer + desplazamiento, &(response->offset), sizeof(off_t));
	desplazamiento += sizeof(off_t);
	memcpy(buffer + desplazamiento, &(response->pathLen), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, &(response->bufLen), sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(buffer + desplazamiento, path, response->pathLen);
	desplazamiento += response->pathLen;
	memcpy(buffer + desplazamiento, bufWrite, response->bufLen);
	//printf(MAG "\t En serializarPedidoWrite el path es:%s\n" RESET, path);
	return buffer;
}

int enviarEstructuraWrite(int fdReceptor, int head, char* path, char* bufWrite, t_writebuf* mensaje)
{
	int desplazamiento = 0;
	//int tamanioMensaje = sizeof(t_writebuf) + strlen(path) + strlen(bufWrite) + 2;
	int tamanioMensaje = sizeof(t_writebuf) + strlen(path) + mensaje->bufLen + 1;
	int tamanioTotalAEnviar = 0;
	//printf(MAG "\t En enviarEstructuraWrite path:%s\n ",path);
	void *mensajeSerializado = serializarPedidoWrite( mensaje, path, bufWrite);

	// Lo que se envía es: head + tamaño del msj serializado:
	tamanioTotalAEnviar = sizeof(int) + tamanioMensaje;

	// Agrego todo junto al buffer para enviar
	void *buffer = malloc(tamanioTotalAEnviar);
	memcpy(buffer + desplazamiento, &head, sizeof(int));
	desplazamiento +=  sizeof(int);
	memcpy(buffer + desplazamiento, mensajeSerializado, tamanioMensaje);

	// Envío la totalidad del paquete (lo contenido en el buffer):
	int enviados = enviarPorSocket(fdReceptor, buffer, tamanioTotalAEnviar);
	//printf("\t En enviarEstructuraWrite enviados:%d\n" RESET, enviados);

	free(mensajeSerializado);
	mensajeSerializado = NULL;
	free(buffer);
	buffer = NULL;

	return enviados;
}
