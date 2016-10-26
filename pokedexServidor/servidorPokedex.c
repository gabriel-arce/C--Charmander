/*
* servidorPokedex.c
*
*  Created on: 29/9/2016
*      Author: David
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
#include "disco.c"

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

//borrar esto------------------------------------------------
struct stat pikachuStat;
struct stat squirtleStat;
struct stat bulbasaurStat;
int* pmap_pikachu;
int* pmap_squirtle;
int* pmap_bulbasaur;

//-----------------------------------------------------------
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
					printf(CYN "\t procesando PEDIDO_CREATE\n" RESET);

					respuesta = procesarPedidoCreate((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_CREATE, respuesta);
						printf(CYN "\t devolviendo RESPUESTA_CREATE\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(CYN "\t devolviendo respuesta ENOENT \n" RESET);
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
						printf(MAG "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_READ:
					printf( "\t procesando PEDIDO_READ\n" );

					void *buffer = NULL;
					buffer = recibirEstructuraRead(socket, &head);

					respuesta = procesarPedidoRead(buffer);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_READ, respuesta);
						printf("\t devolviendo RESPUESTA_READ \n");
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf("\t devolviendo respuesta ENOENT \n");
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
						printf(BLU "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_RMDIR:
					printf(GRN "\t procesando PEDIDO_RMDIR\n" RESET);

					respuesta = procesarPedidoRmdir((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_RMDIR, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_RMDIR\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(GRN "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_TRUNCATE:
					printf(YEL "\t procesando PEDIDO_TRUNCATE\n" RESET);

					respuesta = procesarPedidoTruncate((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_TRUNCATE, respuesta);
						printf(YEL "\t devolviendo RESPUESTA_TRUNCATE\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_UNLINK:
					printf(RED "\t procesando PEDIDO_UNLINK\n" RESET);

					respuesta = procesarPedidoUnlink((char*)pedido);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_UNLINK, respuesta);
						printf(RED "\t devolviendo RESPUESTA_UNLINK\n" RESET);
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf(RED "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_WRITE:
					printf( "\t procesando PEDIDO_WRITE\n" );

					void *bufWrite = NULL;
					bufWrite = recibirEstructuraWrite(socket, &head);

					respuesta = procesarPedidoWrite(bufWrite);
					if(respuesta != NULL)
					{
						enviarConProtocolo(socket, RESPUESTA_WRITE, respuesta);
						printf("\t devolviendo RESPUESTA_WRITE \n");
					}
					else
					{
						enviarConProtocolo(socket, ENOENTRY, pedido);
						printf("\t devolviendo respuesta ENOENT \n");
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
	void *respuesta = NULL;

	t_stbuf* stbuf = malloc(sizeof( t_stbuf));
	memset(stbuf, 0, sizeof( t_stbuf));

	if (strcmp(path, "/") == 0)
	{
		stbuf->mode = S_IFDIR | 0755;
		stbuf->nlink = 2;
		stbuf->size = 0;
	}
	else if (strcmp(path, "/pikachu") == 0)
	{
		stbuf->mode = S_IFDIR | 0755;
		stbuf->nlink = 2;
		stbuf->size = 0;
	}
	else if (strcmp(path, "/squirtle") == 0)
	{
		stbuf->mode = S_IFDIR | 0755;
		stbuf->nlink = 2;
		stbuf->size = 0;
	}
	else if (strcmp(path, "/bulbasaur") == 0)
	{
		stbuf->mode = S_IFDIR | 0755;
		stbuf->nlink = 2;
		stbuf->size = 0;
	}
	else if (strcmp(path, "/pokemon.txt") == 0)
	{
		stbuf->mode = S_IFREG | 0444;
		stbuf->nlink = 1;
		stbuf->size = 144;
	}
	else if (strcmp(path, "/pikachu/pika-chu.mp4") == 0)
	{
		stbuf->mode = S_IFREG | 0444;
		stbuf->nlink = 1;
		stbuf->size = pikachuStat.st_size;
	}
	else if (strcmp(path, "/squirtle/vamo a calmarno.jpg") == 0)
	{
		stbuf->mode = S_IFREG | 0444;
		stbuf->nlink = 1;
		stbuf->size = squirtleStat.st_size;
	}
	else if (strcmp(path, "/bulbasaur/bulbasaur.mp3") == 0)
	{
		stbuf->mode = S_IFREG | 0444;
		stbuf->nlink = 1;
		stbuf->size = bulbasaurStat.st_size;
	}
	else if (strcmp(path, "/pepito.txt") == 0)
	{
		stbuf->mode = S_IFREG | 0444;
		stbuf->nlink = 1;
		stbuf->size = 5;
	}
	else
	{
		printf(YEL "\n\t Path no encontrado \n" RESET);
		free(stbuf);
		return NULL;
	}

	respuesta = malloc(sizeof(t_stbuf));
	memset(respuesta, 0, sizeof(t_stbuf));
	memcpy(respuesta, stbuf, sizeof(t_stbuf));

	free(stbuf);

	return respuesta;
}

void* procesarPedidoReaddir(char *path)
{
	printf("\t path: %s\n", path);
	void *respuesta = NULL;

	if (strcmp(path, "/") == 0)
	{
		char* archivos = "pikachu/squirtle/bulbasaur/pokemon.txt/pepito.txt/";
		respuesta = malloc(strlen(archivos)+1);
		memset(respuesta, 0, strlen(archivos)+1);
		memcpy(respuesta, archivos, strlen(archivos)+1);
		//free(archivos);
	}
	else if (strcmp(path, "/pikachu") == 0)
	{
		char* archivos = "pika-chu.mp4/";
		respuesta = malloc(strlen(archivos)+1);
		memset(respuesta, 0, strlen(archivos)+1);
		memcpy(respuesta, archivos, strlen(archivos)+1);
		//free(archivos);
	}
	else if (strcmp(path, "/squirtle") == 0)
	{
		char* archivos = "vamo a calmarno.jpg/";
		respuesta = malloc(strlen(archivos)+1);
		memset(respuesta, 0, strlen(archivos)+1);
		memcpy(respuesta, archivos, strlen(archivos)+1);
		//free(archivos);
	}
	else if (strcmp(path, "/bulbasaur") == 0)
	{
		char* archivos = "bulbasaur.mp3/";
		respuesta = malloc(strlen(archivos)+1);
		memset(respuesta, 0, strlen(archivos)+1);
		memcpy(respuesta, archivos,strlen(archivos)+1);
		//free(archivos);
	}
	else
	{
		printf(YEL "\n\t Path no encontrado \n" RESET);
		return NULL;
	}

	return respuesta;
}

void* procesarPedidoRead(void* buffer)//en construccion
{
	printf(YEL "\n\t Entre procesarPedidoRead\n" RESET);
	int desplazamiento = 0;
	size_t* size = malloc(sizeof(size_t));
	off_t* offset = malloc(sizeof(off_t));
	int* pathLen = malloc(sizeof(int));

	memcpy(size, buffer , sizeof(size_t));
	desplazamiento += sizeof(size_t);
	memcpy(offset, buffer + desplazamiento, sizeof(off_t));
	desplazamiento += sizeof(off_t);
	memcpy(pathLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	char* path = malloc(pathLen);
	memcpy(path,  buffer + desplazamiento, *pathLen);

	printf(CYN "\n\t En procesarPedidoRead el size es: %d\n", *size);
	printf( "\t En procesarPedidoRead el offset es: %d\n", *offset);
	printf( "\t En procesarPedidoRead el pathlen es: %d\n", *pathLen);
	printf( "\t En procesarPedidoRead el path es: %s\n" RESET, path);

	if (strcmp(path, "/pikachu/pika-chu.mp4") == 0)
	{
		void* respuesta = malloc(*size);
		memset(respuesta, 0, *size);
		memcpy(respuesta,(pmap_pikachu + *offset), *size);
		return respuesta;
	}
	else if (strcmp(path, "/squirtle/vamo a calmarno.jpg") == 0)
	{
		//----------------------
		int fd_squirtle;
		fd_squirtle = open("/fuse_pokemon/vamo a calmarno.jpg",O_RDWR);
		fstat(fd_squirtle,&squirtleStat);
		pmap_squirtle = mmap(0, squirtleStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_squirtle, 0);
		//----------------------

		void* respuesta = malloc(squirtleStat.st_size);
		memset(respuesta, 0, squirtleStat.st_size);
		//memcpy(respuesta,(pmap_squirtle), squirtleStat.st_size);
		memcpy(respuesta,((char*)pmap_squirtle + *offset), squirtleStat.st_size);
		//printf(GRN "\n\n\nvamo a calmarno.jpg: %s\n\n\n" RESET, respuesta);
		return respuesta;
	}
	else if (strcmp(path, "/bulbasaur/bulbasaur.mp3") == 0)
	{
		void* respuesta = malloc(*size);
		memset(respuesta, 0, *size);
		memcpy(respuesta,(pmap_bulbasaur + *offset), *size);
		return respuesta;
	}
	else if (strcmp(path, "/pokemon.txt") == 0)
	{
		printf(GRN "size: %d" RESET, *size);
		void* respuesta = malloc(144);
		memset(respuesta, 0, 144);
		memcpy(respuesta,"Los pokemon son una clase de criaturas (monstruos) basadas en muchos casos en animales reales o criaturas míticas y mitológicas orientales.\n", 144);
		//printf(GRN "respuesta: %s" RESET, respuesta);
		return respuesta;
	}
	else if (strcmp(path, "/pepito.txt") == 0)
	{
		printf(GRN "size: %d" RESET, *size);
		void* respuesta = malloc(5);
		memset(respuesta, 0, 5);
		memcpy(respuesta,"Hola\n", 5);
	//	printf(GRN "respuesta: %s" RESET, respuesta);
		return respuesta;
	}
	else
	{
		printf(RED "\n\t No encontré el path, aca no queria entrar !!!!!\n" RESET);
	}

	//free(buffer);

	return NULL;
}

void* procesarPedidoCreate(char *path)//const char *path, mode_t mode,
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = crearArchivo(path, 0);
	return respuesta;
}

void* procesarPedidoMkdir(char *path)//const char *path, mode_t mode
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = crearArchivo(path, 1);
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
char borrarArchivo(char* path)
{
	//TODO: lee la tabla de archivos, pone en cero/borrado el estado del archivo y actualiza el bitmap
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	return 's';
	//return 'n';
}
char borrarDirectorio(char* path)
{
	//TODO: lee la tabla de archivos, chequea que el directorio este vacio y pone en cero/borrado el estado del directorio
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	return 's';
	//return 'n';
}

char crearArchivo(char* path, int modo)
{
	//TODO: chequeo que exista espacio en la tabla de archivos, si hay agrego el pedido
	// if(modo == 0) //agrego un archivo a la tabla de archivos
		// else //agrego un directorio
	return 's';
	//return 'n';
}

char renombrarArchivo(char* paths)
{
	//TODO: separa el path recibido en nuevo y viejo, lee la tabla de archivos y actualiza el nombre,
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	return 's';
	//return 'n';
}

void asignarOffsets()
{
	int tamanioTablaAsig = oheader.fs_blocks - 1025 - oheader.bitmap_blocks - oheader.data_blocks;
	offsetBitmap = OSADA_BLOCK_SIZE;
	offsetTablaArchivos = OSADA_BLOCK_SIZE + (oheader.bitmap_blocks * OSADA_BLOCK_SIZE);
	offsetAsignaciones = offsetTablaArchivos + 1024;
	offsetDatos = offsetAsignaciones + (sizeof(int) * tamanioTablaAsig);
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

int existePath(char* path, uint16_t** pos)
{
	int i;
	int existe = 1;
	uint16_t* padre;
	padre = 65535;
	printf( "path %s\n", path);
	char *token = malloc(strlen(path)+1);
	char *pathRecibido = malloc(strlen(path)+1);

	strcpy(pathRecibido, path);
	token = strtok(pathRecibido, "/");

	while ((token != NULL) && (existe != 0))
	{
		existe = existeDirectorio(token, &padre,&(*pos));
		token = strtok(NULL, "/");
	}
	//free(pathRecibido);
	if (existe == 0)
	{
		return 0;//no existe
	}
	return 1;
}

int existeDirectorio(unsigned char* token, uint16_t* padre, int* posicion)
{
	osada_file archivo;
	int i;

		for(i=0; i< 2048; i++)
		{
			*posicion=i;
			leerArchivo(i, &archivo);
			if (strcmp(archivo.fname, token) == 0)
			{
				if (archivo.parent_directory == *padre)
				{
				*padre = i;
				}
			if (archivo.state == 2)
			{
				return 1;
			}
			}
		}
		printf(YEL "recorrió toda la tabla de archivos y salió sin encontrar\n" RESET);

		return 0;
}

void inicializarDisco()
{
//para inicializar el disco--------------------
	mapearDisco("basic.bin"); //mapearDisco("challenge.bin");
	leerHeader();
	asignarOffsets();
	//leerTablaArchivos();
//---------------------------------------------

//mas adelante borrar esto y leer los archivos de osada----------------------
//	char* path = "/directorio";
//	char* archivosEnDirectorio = readdir(path);

	//pikachu
	int fd_pikachu;
	//fd_pikachu= open("/home/utnso/fuse_pokemon/pika-chu.mp4",O_RDWR);
	fd_pikachu= open("/fuse_pokemon/pika-chu.mp4",O_RDWR);
	fstat(fd_pikachu,&pikachuStat);
	pmap_pikachu= mmap(0, pikachuStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_pikachu, 0);

	//bulbasaur
	int fd_bulbasaur;
	//fd_bulbasaur= open("/home/utnso/fuse_pokemon/bulbasaur.mp3",O_RDWR);
	fd_bulbasaur= open("/fuse_pokemon/bulbasaur.mp3",O_RDWR);
	fstat(fd_bulbasaur,&bulbasaurStat);
	pmap_bulbasaur= mmap(0, bulbasaurStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_bulbasaur, 0);

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

/* para procesar pedido readdir(),
* recibo un path de fuse y chequeo que exista
* si existe armo una cadena con los nombres de todos los archivos y directorios contenidos en ese path */
void* readdir(char* path)
{
	osada_file archivo;
	unsigned char* seleccionado;
	char* buffer = NULL;
	int* pos;
		int i;
		int contadorArchivosEnPath = 0;

	int existe = existePath(path, &pos);

	if (existe != 0) //si el path es valido busco cuantos archivos contiene para dimensionar la respuesta
	{
		for(i=0; i< 2048; i++)
		{
			leerArchivo(i, &archivo);

			if ((pos == archivo.parent_directory) && (archivo.state != 0))
			{
				contadorArchivosEnPath++;
				printf(CYN "Contar archivo: %s\n" RESET, archivo.fname);
			}
		}
	}
	else
	{
		printf(YEL "no hay archivos para devolver, el path no existe \n" RESET);
		return NULL;
	}

	printf("Cantidad de archivos en path: %d\n", contadorArchivosEnPath);

	buffer = malloc(contadorArchivosEnPath * ((sizeof(char) * OSADA_FILENAME_LENGTH) + 1));//le sumo 1 para agregar el caracter centinela despues de cada fname
	memset(buffer, 0, contadorArchivosEnPath * ((sizeof(char) * OSADA_FILENAME_LENGTH) + 1));

	for(i=0; i< 2048; i++) //armo la cadena que voy a enviar a fuse con los archivos y diectorios encontrados en el path
	{
		leerArchivo(i, &archivo);

		if ((pos == archivo.parent_directory) && (archivo.state != 0))
		{
			strcat(buffer, archivo.fname);
			strcat(buffer, "/");
						printf(CYN "archivo pedido: %s\n" RESET, archivo.fname);
		}
	}
		printf("La cadena de archivos para enviar al cliente es: %s\n", buffer);

	return buffer;
}

//funciones para probar la lectura correcta del disco----------------------------------------------------------------------
void leerTablaArchivos()
{
		osada_file archivo;
		int i;
		printf("tabla de archivos\n");
		for(i=0; i< 15; i++)
		{
			leerArchivo(i, &archivo);
			printf("%s\t %d\t %d\t %d\n", archivo.fname, archivo.parent_directory, archivo.file_size, archivo.state);
//       if(i<7)//sacar esto, esta solo para probar el escribir archivos
//       {
//    	   strcpy(archivo.fname,"Me modificaron");
//    	   escribirArchivo(i, &archivo);
//    	   printf(YEL "%s\t %d\t %d\t %d\n" RESET, archivo.fname, archivo.parent_directory, archivo.file_size, archivo.state);
//       }
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

//-----------------------esto esta para probar o sacar, deberia servir para armar los bloques del archivo
void* concatenate(void *buffer, int tamBuffer, void *tmpBuffer, int tamTmpBuffer, void* result)
{
		// copy buffer to "result"
		memcpy(result, buffer, tamBuffer);
		// copy tmpBuffer to "result" after varA
		memcpy(result + tamBuffer, tmpBuffer, tamTmpBuffer);
}

void* readData(int cant_blocks, int* fat_values, void *buffer)
{
		int i;
		void *tmp_buffer;

		for(i=0; i < cant_blocks; i++)
		{
				leerDato(fat_values[i], &tmp_buffer);
				void* buffer_aux = malloc(OSADA_BLOCK_SIZE * (i + 1) + OSADA_BLOCK_SIZE);
				concatenate(&buffer, OSADA_BLOCK_SIZE * (i + 1), &tmp_buffer, OSADA_BLOCK_SIZE, &buffer_aux);
				buffer = buffer_aux;
		}
}

void* readFile(osada_file ofile, void *buffer)
{
	int i,
	fat_size = offsetDatos - offsetAsignaciones,
	//Redondeo para arriba porque no se puede tener porcion de bloque
	//Ej: int a = (59 + (4 - 1)) / 4; redondea a 15
	cant_blocks = (ofile.file_size + (OSADA_BLOCK_SIZE - 1)) / OSADA_BLOCK_SIZE;
	int fat_values[cant_blocks - 1];
	int next_block = ofile.first_block;
	for (i=0;i < cant_blocks; i++)
	{
				fat_values[i] = next_block;
				leerAsignacion(next_block, &next_block);
	}

	readData(cant_blocks,fat_values, &buffer);

}
