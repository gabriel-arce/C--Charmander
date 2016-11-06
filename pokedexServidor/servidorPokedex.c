/*
* servidorPokedex.c
*
*  Created on: 29/9/2016
*      Author: Guadalupe
*/
#include <commons/bitarray.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <comunicacion.h>

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <osada.h>
#include "servidorPokedex.h"

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

t_log logServidor;
int	listenningSocket;
pthread_mutex_t mutex_comunicacion  = PTHREAD_MUTEX_INITIALIZER;



int main(int argc, char ** argv)
{
	printEncabezado();
	inicializarDisco();

	pthread_mutex_lock(&mutex_comunicacion);
		listenningSocket = crearServer(PUERTO);
	pthread_mutex_unlock(&mutex_comunicacion);

	printf("\n****************** Creando hilos servidores ********************************\n\n");
	pthread_t thread1, thread2, thread3, thread4, thread5;

	pthread_create(&thread1, NULL, hiloComunicacion, NULL);
	pthread_create(&thread2, NULL, hiloComunicacion, NULL);
	pthread_create(&thread3, NULL, hiloComunicacion, NULL);
	pthread_create(&thread4, NULL, hiloComunicacion, NULL);
	pthread_create(&thread5, NULL, hiloComunicacion, NULL);

	signal(SIGINT,terminar);
	signal(SIGTERM,terminar);

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	pthread_join(thread3, NULL);
	pthread_join(thread4, NULL);
	pthread_join(thread5, NULL);

	close(listenningSocket);
	descargar();

	printTerminar();
	return(0);
}

void* hiloComunicacion(void* arg)
{
	int head;
	void *mensaje = NULL;

	printf( "****************** Un hilo empieza a escuchar conexiones *******************\n\n" );

	while(1)
	{
		pthread_mutex_lock(&mutex_comunicacion);
			int socketCliente = aceptarConexion(listenningSocket);
		pthread_mutex_unlock(&mutex_comunicacion);

		mensaje = recibir(socketCliente,&head);
		char* mensajeHSK = mensaje;
		printf("\t Recibiendo pedido de un cliente:%s \n ", mensajeHSK);

		if (mensajeHSK)
		{
			if (enviar(socketCliente, HANDSHAKE, mensajeHSK) == -1)
			{
				printf(YEL "\t El cliente %d se desconecto antes de recibir el handshake \n " RESET, socketCliente);
			}
			else
			{
				printf("\t Devolviendo mensajeHSK %d %s \n", socketCliente, mensajeHSK);
				atendercliente(socketCliente);
			}
		}
		else
		{
			printf(YEL "\t Recibi una propuesta indecente de: %d :%s \n" RESET, socketCliente, mensajeHSK);
		}
	}
}

void atendercliente(int socket)
{
	int continuar = 1;
	while(continuar)
	{
		printf("******** Atiendo al cliente %d *********************************************\n", socket);

		void *respuesta = NULL;
		void *pedido = NULL;
		int head = 0;

		pedido = recibir(socket, &head);
		printf("\n******** Recibi del cliente " CYN "%d" RESET " un mensaje...\n", socket);

		if(pedido != NULL)
		{
			switch(head)
			{
				case PEDIDO_CREATE:
					printf(MAG "\t procesando PEDIDO_CREATE\n" RESET);

					respuesta = procesarPedidoCreate((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_CREATE, respuesta);
						printf(MAG "\t devolviendo RESPUESTA_CREATE\n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_GETATTR:
					printf("\t procesando PEDIDO_GETATTR\n");

					respuesta = procesarPedidoGetatrr((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_GETATTR, respuesta);
						printf("\t devolviendo RESPUESTA_GETATTR \n");
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf("\t devolviendo respuesta ENOENT \n");
					}
					break;

				case PEDIDO_MKDIR:
					printf(MAG "\t procesando PEDIDO_MKDIR\n" RESET);

					respuesta = procesarPedidoMkdir((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_MKDIR, respuesta);
						printf(MAG "\t devolviendo RESPUESTA_MKDIR\n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_OPEN:
					printf(GRN "\t procesando PEDIDO_OPEN\n" RESET);

					respuesta = procesarPedidoOpen((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_OPEN, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_OPEN\n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_READ:
					printf( BLU "\t procesando PEDIDO_READ\n" RESET);

					void *buffer = NULL;
					uint32_t* tamanioBuffer = malloc(sizeof(uint32_t));
					memset(tamanioBuffer, 0, sizeof(uint32_t));

					buffer = recibirEstructuraRead(socket, &head);
					respuesta = procesarPedidoRead(buffer, tamanioBuffer);

					if(respuesta != NULL)
					{
						enviarRespuestaRead(socket, RESPUESTA_READ, respuesta, tamanioBuffer);
						printf(BLU "\t devolviendo RESPUESTA_READ \n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_READDIR:
					printf("\t procesando PEDIDO_READDIR\n");

					respuesta = procesarPedidoReaddir((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_READDIR, respuesta);
						printf("\t devolviendo RESPUESTA_READDIR\n");
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf("\t devolviendo respuesta ENOENT \n");
					}
					break;

				case PEDIDO_RELEASE:
					printf(GRN "\t procesando PEDIDO_RELEASE\n" RESET);

					respuesta = procesarPedidoRelease((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_RELEASE, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_RELEASE\n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_RENAME:
					printf(BLU "\t procesando PEDIDO_RENAME\n" RESET);
					respuesta = procesarPedidoRename((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_RENAME, respuesta);
						printf(BLU "\t devolviendo RESPUESTA_RENAME\n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_RMDIR:
					printf(GRN "\t procesando PEDIDO_RMDIR\n" RESET);

					respuesta = procesarPedidoRmdir((char*)pedido);
					if (respuesta != NULL)
					{
						enviar(socket, RESPUESTA_RMDIR, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_RMDIR\n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_TRUNCATE:
					printf(RED "\t procesando PEDIDO_TRUNCATE\n" RESET);

					off_t *newSize = NULL;
					newSize = (off_t*)recibir(socket, &head);

					if (head == PEDIDO_TRUNCATE_NEW_SIZE)
					{
						respuesta = procesarPedidoTruncate(*newSize, (char*)pedido);
					}
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_TRUNCATE, respuesta);
						printf(RED "\t devolviendo RESPUESTA_TRUNCATE\n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_UNLINK:
					printf(GRN "\t procesando PEDIDO_UNLINK\n" RESET);

					respuesta = procesarPedidoUnlink((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_UNLINK, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_UNLINK\n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_WRITE:
					printf( RED "\t procesando PEDIDO_WRITE\n" RESET);

					void *bufWrite = NULL;
					bufWrite = recibirEstructuraWrite(socket, &head);

					respuesta = procesarPedidoWrite(bufWrite);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_WRITE, respuesta);
						printf(RED "\t devolviendo RESPUESTA_WRITE \n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				default:
					printf(RED "\n¿Porqué entre en default???, ¿tenia que enviar un handshake por segunda vez??? \n\n" RESET);
					enviar(socket,HANDSHAKE, pedido);
					break;
			}

//			free(pedido);
//			free(respuesta);

		}
		else
		{
			printf(YEL "\n******** Se desconecto el cliente %d, buscando uno nuevo para atender ******\n" RESET, socket);
			continuar = 0;
		}

	}//fin while
}

//funciones de servidor para atender pedidos de cliente--------------------------------------------
void printTerminar()
{
	printf("\n\n\n**********************************************************************************\n");
	printf("****************** El servidor cierra la conexion ********************************\n");
	printf("**********************************************************************************\n");
	printf("****************** " GRN "Terminar" RESET " ******************************************************\n");
	printf("**********************************************************************************\n");
	printf(GRN "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n" RESET);
}

void printEncabezado()
{
	printf(GRN "\n\n\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" RESET);
	printf("**********************************************************************************\n");
	printf("**************** " GRN "POKEDEX SERVIDOR" RESET " ************************************************\n");
	printf("**********************************************************************************\n\n");

	printf("**************** Iniciando servidor..\n\n");
}

void* procesarPedidoCreate(char *path)//const char *path, mode_t mode,
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = crearArchivo(path, 1);
	return respuesta;
}

void* procesarPedidoGetatrr(char *path)
{
	printf("\t path: %s\n", path);
	return getAttr(path);
}

void* procesarPedidoMkdir(char *path)//const char *path, mode_t mode
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = crearArchivo(path, 2);
	return respuesta;
}

void* procesarPedidoOpen(char* path)
{
	printf("\t path: %s\n", path);
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = abrirArchivo(path);
	return respuesta;
}

void* procesarPedidoRelease(char* path)
{
	printf("\t path: %s\n", path);
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = liberarArchivo(path);
	return respuesta;
}

void* procesarPedidoRead(void* buffer, uint32_t* tamanioBuffer)
{
	int desplazamiento = 0;
	size_t* size = malloc(sizeof(size_t));
	off_t* offset = malloc(sizeof(off_t));
	int pathLen = 0;

	memset(size, 0, sizeof(size_t));
	memset(offset, 0, sizeof(off_t));

	memcpy(size, buffer, sizeof(size_t));
	desplazamiento += sizeof(size_t);
	memcpy(offset, buffer + desplazamiento, sizeof(off_t));
	desplazamiento += sizeof(off_t);
	memcpy(&pathLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	char* path = malloc(pathLen);
	memset(path, 0, pathLen);
	memcpy(path,  buffer + desplazamiento, pathLen);

	printf(CYN "\t En procesarPedidoRead el size es: %d bytes\n", *size);
	printf( "\t En procesarPedidoRead el offset es: %d bytes\n", (uint32_t)*offset);

	int posicion = -1;
	if(existePath(path, &posicion))
	{
		osada_file* archivo = buscarArchivo(path, &posicion);

		if (archivo == NULL)
		{
			printf(RED "\t En pedido read: No se encontro el archivo: %s\n" RESET, nombre(path));
			return NULL;
		}

		void* archivoCompleto = readFile(archivo);
		uint32_t bytes = archivo->file_size - (uint32_t)*offset;
		printf(BLU "\t El size del archivo completo es: %d bytes\n", archivo->file_size);

		if ((bytes <= *size) && (*offset == 0))
		{
			//printf(BLU "\t Los bytes en (bytes <= *size) son: %d bytes\n", bytes);
			memcpy(tamanioBuffer, &bytes, sizeof(uint32_t));
			return archivoCompleto;
		}
		else if (bytes <= *size)
		{
			//printf(BLU "\t Los bytes en (bytes <= *size)son: %d bytes y offset >0\n", bytes);
			void* respuesta = malloc(bytes);
			memset(respuesta, 0, bytes);
			memcpy(tamanioBuffer, &bytes, sizeof(uint32_t));
			memcpy(respuesta, archivoCompleto + *offset , bytes);
			return respuesta;
		}
	//	printf(CYN "\t Los bytes en (bytes > *size)son: %d bytes\n", bytes);
		void* respuesta = malloc(*size);
		memset(respuesta, 0, *size);
		memcpy(tamanioBuffer, size, sizeof(uint32_t));
		memcpy(respuesta, archivoCompleto + *offset , *size);
		return respuesta;
	}
	else
	{
		printf(RED "\n\t No encontré el path!\n" RESET);
		return NULL;
	}

	//free(buffer);
	printf(RED "\t NO DEBERIA ENTRAR ACA\n" RESET);
	return NULL;
}

void* procesarPedidoReaddir(char *path)
{
	printf("\t path: %s\n", path);
	return readdir(path);
}

void* procesarPedidoRename(char *paths)//path - newpath
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = renombrarArchivo(paths);
	return respuesta;
}

void* procesarPedidoRmdir(char *path)
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = borrarDirectorio(path);
	return respuesta;
}

void* procesarPedidoTruncate(off_t newSize, char* path)
{
	printf(CYN "\t En procesarPedidoTruncate el nuevo size es: %d\n", (uint32_t)newSize);
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = truncar(path, (uint32_t)newSize);
	return respuesta;
}

void* procesarPedidoUnlink(char* path)
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = borrarArchivo(path);
	return respuesta;
}

void* procesarPedidoWrite(void *buffer)
{
	int desplazamiento = 0;
	size_t* size = malloc(sizeof(size_t));
	off_t* offset = malloc(sizeof(off_t));
	int pathLen;// = malloc(sizeof(int));
	int* bufLen = malloc(sizeof(int));

	memcpy(size, buffer, sizeof(size_t));
	desplazamiento += sizeof(size_t);
	memcpy(offset, buffer + desplazamiento, sizeof(off_t));
	desplazamiento += sizeof(off_t);
	memcpy(&pathLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(bufLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* path = malloc(pathLen);
	memcpy(path,  buffer + desplazamiento, pathLen);
	desplazamiento += pathLen;
	void* bufWrite = malloc(*bufLen);
	memcpy(bufWrite,  buffer + desplazamiento, *bufLen);

	printf(BLU "\t En procesarPedidoWrite el size es: %d\n", *size);
	printf( "\t En procesarPedidoWrite el offset es: %d\n", (int)*offset);
	//printf( "\t En procesarPedidoWrite el pathlen es: %d\n", pathLen);
//	printf( "\t En procesarPedidoWrite el bufLen es: %d\n", *bufLen);
	//printf( "\t En procesarPedidoWrite el path es: %s\n" RESET, path);

	return writeBuffer(size, offset, path, bufWrite);
}

void liberarRecursos()
{

}

void terminar()
{
	close(listenningSocket);

	descargar();
	liberarRecursos();
	printf(RED "\n\n------------------ Señal SIGTERM -------------------------------------------------\n" RESET);
	printTerminar();

	exit(0);
}
