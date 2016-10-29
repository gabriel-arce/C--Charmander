/*
* servidorPokedex.c
*
*  Created on: 29/9/2016
*      Author: Guadalupe
*/

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

#include "osada.h"
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

//variables de disco
char* disco;
off_t tamanioArchivo;
int32_t descriptorArchivo;
osada_header oheader;
int offsetBitmap;
int offsetTablaArchivos;
int offsetAsignaciones;
int offsetDatos;

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
	descargar(descriptorArchivo);

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


		mensaje = recibirConProtocolo(socketCliente,&head);
		char* mensajeHSK = mensaje;
		printf("\t Recibiendo pedido de un cliente:%s \n ", mensajeHSK);

		if (mensajeHSK)
		{
			if (enviarConProtocolo(socketCliente, HANDSHAKE, mensajeHSK) == -1)
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

		pedido = recibirConProtocolo(socket, &head);
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
						enviarConProtocolo(socket, RESPUESTA_CREATE, respuesta);
						printf(MAG "\t devolviendo RESPUESTA_CREATE\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_GETATTR:
					printf("\t procesando PEDIDO_GETATTR\n");

					respuesta = procesarPedidoGetatrr((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_GETATTR, respuesta);
						printf("\t devolviendo RESPUESTA_GETATTR \n");
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf("\t devolviendo respuesta ENOENT \n");
					}
					break;

				case PEDIDO_MKDIR:
					printf(MAG "\t procesando PEDIDO_MKDIR\n" RESET);

					respuesta = procesarPedidoMkdir((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_MKDIR, respuesta);
						printf(MAG "\t devolviendo RESPUESTA_MKDIR\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_OPEN:
					printf(CYN "\t procesando PEDIDO_OPEN\n" RESET);

					respuesta = procesarPedidoOpen((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_OPEN, respuesta);
						printf(CYN "\t devolviendo RESPUESTA_OPEN\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_READ:
					printf( YEL "\t procesando PEDIDO_READ\n" RESET);

					void *buffer = NULL;
					buffer = recibirEstructuraRead(socket, &head);

					respuesta = procesarPedidoRead(buffer);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_READ, respuesta);
						printf(YEL "\t devolviendo RESPUESTA_READ \n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_READDIR:
					printf("\t procesando PEDIDO_READDIR\n");

					respuesta = procesarPedidoReaddir((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_READDIR, respuesta);
						printf("\t devolviendo RESPUESTA_READDIR\n");
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf("\t devolviendo respuesta ENOENT \n");
					}
					break;

				case PEDIDO_RENAME:
					printf(BLU "\t procesando PEDIDO_RENAME\n" RESET);
					respuesta = procesarPedidoRename((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_RENAME, respuesta);
						printf(BLU "\t devolviendo RESPUESTA_RENAME\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_RMDIR:
					printf(GRN "\t procesando PEDIDO_RMDIR\n" RESET);

					respuesta = procesarPedidoRmdir((char*)pedido);
					if (respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_RMDIR, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_RMDIR\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_TRUNCATE:
					printf(BLU "\t procesando PEDIDO_TRUNCATE\n" RESET);

					respuesta = procesarPedidoTruncate((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_TRUNCATE, respuesta);
						printf(BLU "\t devolviendo RESPUESTA_TRUNCATE\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_UNLINK:
					printf(GRN "\t procesando PEDIDO_UNLINK\n" RESET);

					respuesta = procesarPedidoUnlink((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_UNLINK, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_UNLINK\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
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
						enviarConProtocolo(socket, RESPUESTA_WRITE, respuesta);
						printf(RED "\t devolviendo RESPUESTA_WRITE \n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				default:
					printf(RED "\n¿Porqué entre en default???, ¿tenia que enviar un handshake por segunda vez??? \n\n" RESET);
					enviarConProtocolo(socket,HANDSHAKE, pedido);
					break;
			}

			free(pedido);
			free(respuesta);

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
	printf("****************** " GRN "POKEDEX SERVIDOR" RESET " **********************************************\n");
	printf("**********************************************************************************\n\n");

	printf("****************** Iniciando servidor..\n\n");
}

void* procesarPedidoGetatrr(char *path)
{
	printf("\t path: %s\n", path);
	return getAttr(path);
}

void* procesarPedidoOpen(char* path)
{
	printf("\t path: %s\n", path);
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = abrirArchivo(path);
	return respuesta;
}

void* procesarPedidoReaddir(char *path)
{
	printf("\t path: %s\n", path);
	return readdir(path);
}

void* procesarPedidoRead(void* buffer)
{
	//printf(YEL "\n\t Entre procesarPedidoRead\n" RESET);
	int desplazamiento = 0;
	size_t* size = malloc(sizeof(size_t));
	off_t* offset = malloc(sizeof(off_t));
	int pathLen;

	memcpy(size, buffer , sizeof(size_t));
	desplazamiento += sizeof(size_t);
	memcpy(offset, buffer + desplazamiento, sizeof(off_t));
	desplazamiento += sizeof(off_t);
	memcpy(&pathLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	char* path = malloc(pathLen);
	memcpy(path,  buffer + desplazamiento, pathLen);

//	printf(CYN "\n\t En procesarPedidoRead el size es: %d\n", *size);
//	printf( "\t En procesarPedidoRead el offset es: %d\n", *offset);
//	printf( "\t En procesarPedidoRead el pathlen es: %d\n", pathLen);
//	printf( CYN "\t En procesarPedidoRead el path es: %s\n" RESET, path);

	int posicion;
	if(existePath(path, &posicion))
	{

		osada_file* archivo = buscarArchivo(path, &posicion);
		if (archivo == NULL)
		{
			printf(RED "\t En pedido read: No se encontro el archivo: %s\n" RESET, nombre(path));
			return 'n';
		}

		printf(GRN "\t Archivo leido\n" RESET);
		return readFile(archivo);
	}
	else
	{
		printf(RED "\n\t No encontré el path!\n" RESET);
	}

	//free(buffer);

	return NULL;
}

void* procesarPedidoCreate(char *path)//const char *path, mode_t mode,
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = crearArchivo(path, 1);
	return respuesta;
}

void* procesarPedidoMkdir(char *path)//const char *path, mode_t mode
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = crearArchivo(path, 2);
	return respuesta;
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

void* procesarPedidoUnlink(char* path)
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = borrarArchivo(path);
	return respuesta;
}

void* procesarPedidoTruncate(char *path)//const char *path, off_t new_size
{
	void* respuesta = NULL;

	return respuesta;//ver que devuelve
}

void* procesarPedidoWrite(void *buffer)//en construccion
{
	int desplazamiento = 0;
	size_t* size = malloc(sizeof(size_t));
	off_t* offset = malloc(sizeof(off_t));
	int* pathLen = malloc(sizeof(int));
	int* bufLen = malloc(sizeof(int));

	memcpy(size, buffer, sizeof(size_t));
	desplazamiento += sizeof(size_t);
	memcpy(offset, buffer + desplazamiento, sizeof(off_t));
	desplazamiento += sizeof(off_t);
	memcpy(pathLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(bufLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* path = malloc(pathLen);
	memcpy(path,  buffer + desplazamiento, *pathLen);
	desplazamiento += *pathLen;
	char* bufWrite = malloc(bufLen);
	memcpy(bufWrite,  buffer + desplazamiento, *bufLen);

	printf(CYN "\n\t En procesarPedidoWrite el size es: %d\n", *size);
	printf( "\t En procesarPedidoWrite el offset es: %d\n", *offset);
	printf( "\t En procesarPedidoWrite el pathlen es: %d\n", *pathLen);
	printf( "\t En procesarPedidoWrite el bufLen es: %d\n", *bufLen);
	printf( "\t En procesarPedidoWrite el path es: %s\n" RESET, path);

	if (strcmp(path, "/pokemon.txt") == 0)
	{
		//llamar a funcion que escriba en el archivo
		printf(GRN "Escribir en archivo un buffer de size: %d" RESET, *size);
		void* respuesta = malloc(sizeof(char));
		memcpy(respuesta, 'b', sizeof(char));
		return respuesta;
	}
	else
	{
		printf(RED "\n\t En construccion!!!!!\n" RESET);
	}

	//free(buffer);

	return NULL;

}

void terminar()
{
	close(listenningSocket);
	descargar(descriptorArchivo);
	printf(RED "\n\n------------------ Señal SIGTERM -------------------------------------------------\n" RESET);
	printTerminar();

	exit(0);
}

//funciones de disco----------------------------------------------------------------------------------
char* nombre(char* path)
{
	char* pathToken = malloc(strlen(path)+1);
	strcpy(pathToken, path);

	char* token = malloc(strlen(path) +1);
	char* respuesta = malloc(strlen(path) +1);

	token = strtok(pathToken, "/");

	while (token != NULL)
	{
		strcpy(respuesta, token);
		token = strtok(NULL, "/");
	}
	return respuesta;
}

int posicionUltimoToken(char* path)
{
	int i;
	if(strlen(path) > 1)
	{
		for(i = strlen(path) - 2; i >= 0; i --)
		{
			if(path[i] == '/')
			{
				return i;
			}
		}
	}
	return -1;
}

int padre(char* path)
{
	//printf(YEL "\t Path recibido en padre: %s\n" RESET, path);
	int posicion;

	if((posicion = posicionUltimoToken(path)) == -1)
	{
		printf(RED "\t No se encontro el archivo padre\n" RESET);
		return -1;
	}

	if(posicion == 0)
	{
		return 65535;
	}

	char* pathPadre = malloc((sizeof(char) * posicion) + 1);
	strncpy(pathPadre, path, posicion);
	osada_file* archivo = buscarArchivo(pathPadre, &posicion);
	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo padre\n" RESET);
		return -1;
	}

	return posicion;
}

char borrarArchivo(char* path)
{
	//lee la tabla de archivos, pone en cero/borrado el estado del archivo y actualiza el bitmap
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	int posicion;

	osada_file* archivo = buscarArchivo(path, &posicion);
	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo\n" RESET);
		return 'n';
	}

		archivo->state = 0;
		escribirArchivo(posicion, archivo);

		//TODO: falta actualizar el bitmap

	printf(YEL "\t Se borro el archivo: %s\n" RESET, archivo->fname);
	return 's';
}

int esDirectorioVacio(int posicion)
{
	int i;
	osada_file* archivo = malloc(sizeof(osada_file));

		for(i=0; i< 2048; i++)
		{
			leerArchivo(i, archivo);

			if ((archivo->parent_directory == posicion) && (archivo->state != 0))
			{
				free(archivo);
				return 0;
			}
		}

		free(archivo);
	return 1;
}

char borrarDirectorio(char* path)
{
	//lee la tabla de archivos, chequea que el directorio este vacio y pone en cero/borrado el estado del directorio
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	int posicion;

	osada_file* archivo = buscarArchivo(path, &posicion);
	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo\n" RESET);
		return 'n';
	}
	if (!esDirectorioVacio(posicion))
	{
		printf(RED "\t No se pudo borrar el directorio porque no esta vacio\n" RESET);
		return 'n';
	}
		archivo->state = 0;
		escribirArchivo(posicion, archivo);

	printf(YEL "\t Se borro el archivo: %s\n" RESET, archivo->fname);
	return 's';
}

int buscarEspacioLibre()
{
	int pos;
	int i;
	osada_file* archivo = malloc(sizeof(osada_file));
	memset(archivo, 0, sizeof(osada_file));
	for (i = 0; i< 2048; i++)
	{
		leerArchivo(i, archivo);
		if(archivo->state == 0)
		{
			return i;
		}
	}
	return -1;
}

int agregarArchivo(char* path, int modo)
{
	//printf(CYN "\t nombre(path) en agregarArchivo %s\n" RESET, nombre(path));
	int pos;

	if(strcmp(nombre(path), ".Trash-1000") == 0)
	{
		return -1;
	}

	if ((pos = buscarEspacioLibre()) == -1)// chequeo que exista espacio en la tabla de archivos, si hay agrego el pedido
	{
		printf(RED "\t No hay espacio para crear el archivo %s\n" RESET, nombre(path));
		return -1;
	}

	int p = padre(path);
	if (p == -1)//ver el caso en que el padre sea el directorio raiz
	{
		printf(RED "\t Salgo sin agregar el archivo %s\n" RESET, nombre(path));
		return -1;
	}
	osada_file* nuevo = malloc(sizeof(osada_file));
	memset(nuevo, 0, sizeof(osada_file));
	strcpy(nuevo->fname, nombre(path));
	nuevo->file_size = 0;
	nuevo->lastmod = 0;
	nuevo->first_block = 0;//ver con que inicializar esto
	nuevo->state = modo;//si es archivo o directorio
	nuevo->parent_directory = p;
	escribirArchivo(pos, nuevo);

	return 0;
}

char crearArchivo(char* path, int modo)
{
	int posicion;
	osada_file* archivo = buscarArchivo(path, &posicion);
	//printf(BLU "\t nombre(path) en crearArchivo %s\n" RESET, nombre(path));
	if (archivo == NULL)
	{
		if (agregarArchivo(path, modo) == -1)//si no hay espacio en la tabla de archivos
		{
			return 'n';
		}
		printf(GRN "\t Se creó el archivo: %s\n" RESET, nombre(path));
		return 's';
	}
	else if (archivo->parent_directory != padre(path))
	{
		if (agregarArchivo(path, modo) == -1)
		{
			return 'n';
		}
		printf(GRN "\t Se creó el archivo: %s\n" RESET, nombre(path));
		return 's';
	}
	printf(RED "\t Ya existe el archivo %s\n" RESET, nombre(path));
	return 'n';

}

osada_file* buscarArchivo(char* path, int* posicion)
{
	osada_file* archivo = malloc(sizeof(osada_file));
	int i;

		for(i=0; i< 2048; i++)
		{
			leerArchivo(i, archivo);

			if ((strcmp(archivo->fname, nombre(path)) == 0) && (archivo->state != 0))
			{
				if (archivo->parent_directory == padre(path))
				{
				*posicion = i;
				return archivo;
				}
			}
		}

		return NULL;
}

char abrirArchivo(char* path)
{
	//TODO: chequear que el archivo exista, ver de tener una tabla con archivos abiertos y el modo (lectura o escritura)
	return 's';
}

char renombrarArchivo(char* paths)
{
	//separa el path recibido en nuevo y viejo, lee la tabla de archivos y actualiza el nombre,
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	int posicion;
	//printf(YEL "\t En renombrar: Los paths concatenados son: %s\n" RESET, paths);
	char *viejo = malloc(strlen(paths)+1);
	char *nuevo = malloc(strlen(paths)+1);

	viejo = strtok(paths, "*");
	nuevo = strtok(NULL, "*");

	osada_file* archivo = buscarArchivo(viejo, &posicion);
	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo: %s\n" RESET, viejo);
		return 'n';
	}

		strcpy(archivo->fname, nombre(nuevo));
		escribirArchivo(posicion, archivo);

	printf(GRN "\t Se cambio el nombre del archivo: %s por: %s\n" RESET, nombre(viejo), nombre(nuevo));
	return 's';
}

void asignarOffsets()
{
	int tamanioTablaAsig = (oheader.fs_blocks - 1025 - oheader.bitmap_blocks - oheader.data_blocks) * OSADA_BLOCK_SIZE;
	offsetBitmap = OSADA_BLOCK_SIZE;
	offsetTablaArchivos = OSADA_BLOCK_SIZE + (oheader.bitmap_blocks * OSADA_BLOCK_SIZE);
	offsetAsignaciones = offsetTablaArchivos + (1024 * OSADA_BLOCK_SIZE);
	offsetDatos = offsetAsignaciones + tamanioTablaAsig;
}

void descargar(uint32_t descriptorArchivo)
{
	if (munmap(disco, tamanioArchivo) == -1)
	{
		printf(RED "Error en munmap.\n" RESET);
		return;
	}
	if(close(descriptorArchivo)<0)
	{
		printf(RED "Error en close.\n" RESET);
		return;
	}
}

void escribirBloque(uint32_t bloque, char* buf)
{
	memcpy(disco + (bloque * OSADA_BLOCK_SIZE), buf, OSADA_BLOCK_SIZE);
}

void escribirArchivo(uint32_t posicion, char* buf)
{
	memcpy(disco + offsetTablaArchivos + (posicion * sizeof(osada_file)), buf, sizeof(osada_file));
}

int existePath(char* path, uint16_t* pos)
{
	int i;
	int existe = 1;
	uint16_t padre;
	padre = 65535;
	//printf( "path %s\n", path);

	char *token = malloc(strlen(path)+1);
	char *pathRecibido = malloc(strlen(path)+1);

	strcpy(pathRecibido, path);
	token = strtok(pathRecibido, "/");

	while ((token != NULL) && (existe != 0))
	{
	//	printf(CYN "token: %s\n" RESET, token);
		existe = existeDirectorio(token, &padre, pos);
		token = strtok(NULL, "/");
	}
	//free(pathRecibido);
	if (existe == 0)
	{
		printf(YEL "\t No existe path: %s\n" RESET, path);
		return 0;//no existe
	}
	printf(CYN "\t Existe path: %s, posicion:%d\n" RESET, path, padre);
	return 1;
}

int existeDirectorio(unsigned char* token, uint16_t* padre, int* posicion)
{
	osada_file archivo;
	int i;

		for(i=0; i< 2048; i++)
		{
			*posicion = i;
			leerArchivo(i, &archivo);

			if ((strcmp(archivo.fname, token) == 0) && (archivo.state != 0))
			{
				if (archivo.parent_directory == *padre)
				{
				*padre = i;
				return 1;
			}
			}
		}

		return 0;
}

void inicializarDisco()
{
//para inicializar el disco--------------------
	mapearDisco("challenge.bin"); //mapearDisco("challenge.bin");
	leerHeader();
	asignarOffsets();
	leerTablaArchivos();
//
//////mas adelante borrar esto
//	char* path = "/directorio/subdirectorio";
//	int posicion;
//
//	osada_file* archivo = buscarArchivo(path, &posicion);
//	if (archivo == NULL)
//	{
//		printf(RED "\t  No se encontro el archivo: %s\n" RESET, nombre(path));
//	}
//	else
//	{
//		printf(GRN "\t se encontro el archivo: %s\n" RESET, nombre(path));
//	}


////	char* archivosEnDirectorio = readdir(path);
////	 getAttr(path);
//	printf("%s\n", nombre(path));
//
//	char* path2 = "/directorio/subdirectorio/large.txt";
//////	char* archivosEnDirectorio = readdir(path);
//////	 getAttr(path);
////	printf("%s\n", nombre(path2));
//	int posicion = posicionUltimoToken(path2);
//	char* pathPadre = malloc((sizeof(char) * posicion) + 1);
//	strncpy(pathPadre, path2, posicion);
//	 //padre( path);
//	printf(RED "--path:%s, path padre: %s---" RESET, path2, pathPadre);
}

void leerArchivo(uint32_t posicion, osada_file* buf)
{
	memcpy(buf, disco + offsetTablaArchivos + (posicion * sizeof(osada_file)), sizeof(osada_file));
}

void leerAsignacion(uint32_t posicion, osada_file* buf)
{
	memcpy(buf, disco + offsetAsignaciones + (posicion * sizeof(int)), sizeof(int));
}

void leerBloque(uint32_t cantidadBloques, char* buf)
{
	memcpy(buf, disco + (cantidadBloques * OSADA_BLOCK_SIZE), OSADA_BLOCK_SIZE);
}

void leerDato(uint32_t posicion, osada_file* buf)
{
	memcpy(buf, disco + offsetDatos + (posicion * OSADA_BLOCK_SIZE), OSADA_BLOCK_SIZE);
}

void leerHeader()
{
	memcpy(&oheader, disco, sizeof(osada_header));
	mostrarHeader(oheader);
}

int mapearDisco(char* path)
{
		struct stat sb;

	if((descriptorArchivo = open(path,O_RDWR)) == -1)
	{
		printf(RED "Error en open \n" RESET);
		return -1;
	}

		if(fstat(descriptorArchivo,&sb)== -1)
		{
			printf(RED "Error en stat \n" RESET);
				return -1;
		}

		tamanioArchivo = sb.st_size;
	disco = mmap(NULL,tamanioArchivo, PROT_READ | PROT_WRITE, MAP_SHARED, descriptorArchivo,0);

	if(disco == MAP_FAILED)
	{
		printf(RED "Error en mmap.\n" RESET);
		return -1;
	}

	posix_madvise(disco,tamanioArchivo,POSIX_MADV_SEQUENTIAL);
	return descriptorArchivo;
}

void mostrarHeader(osada_header oheader)
{
		printf(GRN "                   HEADER FILE SYSTEM OSADA \n" RESET);
		char id[6];
		strncpy(id, oheader.magic_number,6);
		printf("                     ID: %s \n", id);
		printf("                     Version: %d \n", oheader.version);
		printf("                     FS size: %d blocks \n", oheader.fs_blocks);
		printf("                     Bitmap size: %d blocks\n", oheader.bitmap_blocks);
		printf("                     Allocations table offset: block #%d \n", oheader.allocations_table_offset);
		printf("                     Data size: %d blocks \n \n", oheader.data_blocks);
}

void* getAttr(char* path)
{
	osada_file archivo;
		int pos = 0;
		int existe = 1;

		if (strcmp(path, "/") == 0)
		{
		t_stbuf* stbuf = malloc(sizeof(t_stbuf));
		stbuf->mode = S_IFDIR | 0755;
		stbuf->nlink = 2;
		stbuf->size = 0;
		return stbuf;
		}

	if (existePath(path, &pos))
	{
		leerArchivo(pos, &archivo);

		if (archivo.state != 0)
		{
			t_stbuf* stbuf = malloc(sizeof( t_stbuf));
			if (archivo.state == 2)//si es un directorio
			{
				stbuf->mode = S_IFDIR | 0755;
				stbuf->nlink = 2;
				stbuf->size = 0;
			}
			else
			{
				stbuf->mode = S_IFREG | 0444;
				stbuf->nlink = 1;
				stbuf->size = archivo.file_size;
			}

			return stbuf;
		}
	}

	return NULL;
}


/* para procesar pedido readdir(),
* recibo un path de fuse y chequeo que exista
* si existe armo una cadena con los nombres de todos los archivos y directorios contenidos en ese path */
void* readdir(char* path)
{
	osada_file archivo;
	unsigned char* seleccionado;
	char* buffer = NULL;
	int pos;
		int i;
		int existe = 1;
		int contadorArchivosEnPath = 0;

		if (strcmp(path, "/") == 0)
		{
			pos = 65535;
		}
		else
		{
			existe = existePath(path, &pos);
		}

	if (existe != 0) //si el path es valido busco cuantos archivos contiene para dimensionar la respuesta
	{
		for(i=0; i< 2048; i++)
		{
			leerArchivo(i, &archivo);

			if ((pos == archivo.parent_directory) && (archivo.state != 0))
			{
				contadorArchivosEnPath++;
			}
		}
	}
	else
	{
		//printf(YEL "El path no existe \n" RESET);
		return NULL;
	}

	printf("\t Cantidad de archivos en path: %d\n", contadorArchivosEnPath);

	if(contadorArchivosEnPath != 0)
	{
		buffer = malloc(contadorArchivosEnPath * ((sizeof(char) * OSADA_FILENAME_LENGTH) + 1));//le sumo 1 para agregar el caracter centinela despues de cada fname
		memset(buffer, 0, contadorArchivosEnPath * ((sizeof(char) * OSADA_FILENAME_LENGTH) + 1));

		for(i=0; i< 2048; i++) //armo la cadena que voy a enviar a fuse con los archivos y diectorios encontrados en el path
		{
			leerArchivo(i, &archivo);

			if ((pos == archivo.parent_directory) && (archivo.state != 0))
			{
				strcat(buffer, archivo.fname);
				//printf(MAG "\t Archivo para el path: %s\n" RESET, archivo.fname);
				strcat(buffer, "/");
			}
		}
		printf(CYN "\t Archivos en path: %s\n" RESET, buffer);
	}

	return buffer;
}

//funciones para probar la lectura correcta del disco----------------------------------------------------------------------
void leerTablaArchivos()
{
		osada_file archivo;
		int i;
		printf("tabla de archivos\n");
		printf("    Archivo.fname  parent_directory  file_size  state\n");
		for(i=0; i< 20; i++)
		{
			leerArchivo(i, &archivo);
			printf("%17s\t %8d\t %4d\t %4d\n", archivo.fname, archivo.parent_directory, archivo.file_size, archivo.state);
		}
}

void leerTablaAsignaciones()
{
		int asignacion;
		int i;
		printf("tabla de asignaciones\n");
		for(i=0; i< 100; i++)
		{
			leerAsignacion(i, &asignacion);
				printf("%d \n", asignacion);
		}
}

void leerTablaDatos()
{
	osada_block bloque;
		int i;
		printf("tabla de datos\n");
		for(i=0; i< 100; i++)
		{
			leerDato(i, &bloque);
			printf("%s \n", bloque);
		}
}

void* readFile(osada_file* archivo)
{
	int i;
	int offset = 0;
	//int fat_size = offsetDatos - offsetAsignaciones;

	int cant_blocks = archivo->file_size / OSADA_BLOCK_SIZE;
	if((archivo->file_size % OSADA_BLOCK_SIZE) != 0)
	{
		cant_blocks++;
	}
	printf(GRN "\t Cantidad de bloques: %d\n" RESET, cant_blocks);
	int next_block = archivo->first_block;
	void *buffer = malloc(OSADA_BLOCK_SIZE * cant_blocks);

		void* bufferAux = malloc(OSADA_BLOCK_SIZE);
	for (i=0; i < cant_blocks; i++)
	{

		//printf(YEL "asignacion: %d" RESET, next_block);
				leerDato(next_block, bufferAux);
				memcpy(buffer + offset, bufferAux, OSADA_BLOCK_SIZE);
				offset += OSADA_BLOCK_SIZE;
				leerAsignacion(next_block, &next_block);
	}

	void* respuesta = malloc(archivo->file_size);
	memcpy(respuesta, buffer, archivo->file_size);

	free(bufferAux);
	free(buffer);
	bufferAux = NULL;
	buffer = NULL;

	return respuesta;
}
