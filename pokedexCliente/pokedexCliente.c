
/*
 * pokedexCliente.c
 *
 *  Created on: 17/9/2016
 *      Author: Guadalupe
 */
#include <comunicacion.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <netdb.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>

#include "pokedexCliente.h"

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

int* socketServidor;
t_log* logCliente;

char mensaje[6] = "\0";
//char* ip;
//char* puerto;

char ip[10];
char puerto[5];

int iret1;
pthread_t thread_Planificador;
pthread_mutex_t mutex_comunicacion  = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
//	ip = getenv("SERVER_HOST");
//	puerto = getenv("SERVER_PORT");
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	int head=0;
	void *mensajeHSK = NULL;

	printEncabezado();

	logCliente = log_create("logPokedex", "Pokedex", false, LOG_LEVEL_DEBUG);
	log_info(logCliente, "****************** Creando archivo Log *******************************************" );

	memset(ip,0,10);
	memset(puerto,0,5);
	memset(mensaje,0,5);

	strcpy(ip,"127.0.0.1");
	strcpy(puerto,"4969");
	strcpy(mensaje,"sock");

	signal(SIGINT,terminar);
	signal(SIGTERM,terminar);

	//pthread_mutex_lock(&mutex_comunicacion);
	socketServidor = malloc(sizeof(int*));

	printf("****************** Estableciendo conexion con el servidor... \n\n");
	log_info(logCliente, "****************** Estableciendo conexion con el servidor... " );

	int socket = crearSocket(ip, puerto);

	if (socket == -1)
	{
		printErrorConexion();
		return -1;
	}
	else if (socket == -2)
	{
		printServidorDesconectado();
		return -1;
	}

	memset(socketServidor,0,sizeof(int*));
	*socketServidor = socket;
	printf("\t Enviando mensaje handshake al servidor \n");
	log_info(logCliente, "	Enviando mensaje handshake al servidor" );

	enviarConProtocolo(*socketServidor, HANDSHAKE, mensaje);
	mensajeHSK = recibirConProtocolo(*socketServidor,&head);
	//pthread_mutex_unlock(&mutex_comunicacion);

	if (head == HANDSHAKE)
	{
		printConectado();
	}
	else
	{
		//no deberia entrar aca
		printMensajeInesperado(head);
	}

	PrintFuse();
	return fuse_main(args.argc, args.argv, &osada_oper, NULL);;
}

static int osada_getattr(const char *path, struct stat *stbuf)
{
	log_info(logCliente, "****************** FUSE: llamada a osada_getattr() ******************************" );
	log_info(logCliente, path);

	memset(stbuf, 0, sizeof(struct stat));

//	//Si path es igual a "/" nos estan pidiendo los atributos del punto de montaje
	if ((strcmp(path, "/.Trash") != 0) && (strcmp(path, "/.Trash-1000") != 0) && (strcmp(path, " /.xdg-volume-info") != 0) && (strcmp(path, "/autorun.inf") != 0) && (strcmp(path, "/.xdg-volume-info") != 0))
	{
		int head = 0;
		t_stbuf *paquete = NULL;

		pthread_mutex_lock(&mutex_comunicacion);
			enviarConProtocolo(*socketServidor, PEDIDO_GETATTR, path);
			log_info(logCliente, "	Envie PEDIDO_GETATTR");
			paquete = (t_stbuf*)recibirConProtocolo(*socketServidor,&head);
		pthread_mutex_unlock(&mutex_comunicacion);

		if (head == RESPUESTA_GETATTR)
		{
//			memcpy(stbuf->st_size, paquete, sizeof(t_stbuf));
			stbuf->st_mode = paquete->mode;
			stbuf->st_nlink = paquete->nlink;
			stbuf->st_size = paquete->size;

			log_info(logCliente, "	Recibi RESPUESTA_GETATTR");
			//log_info(logCliente, stbuf->st_size);

			return 0;
		}
		else
		{
			log_info(logCliente, "	No recibi RESPUESTA_GETATTR");
			return -ENOENT;
		}
	}
	else
	{
		log_info(logCliente, "	Recibi ENOENT");
		return -ENOENT;
	}
}

static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{

	log_info(logCliente, "****************** FUSE: llamada a osada_readdir() ******************************" );
	log_info(logCliente, path);

	int head = 0;
	void *paquete = NULL;

	pthread_mutex_lock(&mutex_comunicacion);
		enviarConProtocolo(*socketServidor, PEDIDO_READDIR, path);
		log_info(logCliente, "	Envie PEDIDO_READDIR");
		paquete = recibirConProtocolo(*socketServidor,&head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_READDIR)
	{
		log_info(logCliente, "	Recibi RESPUESTA_READDIR");
		log_info(logCliente, (char*)paquete);

		char *token = malloc(strlen(paquete)+1);

		token = strtok(paquete, "/");

		while (token != NULL)
		{
			filler(buf, token, NULL, 0);

			log_info(logCliente, (char*)token);
			token = strtok(NULL, "/");
		}

		free(token);
		free(paquete);
		return 0;
	}
	else
	{
		log_info(logCliente, "	Recibi respuesta ENOENT");
		return -ENOENT;
	}
}

static int osada_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	// leer un archivo abierto
	log_info(logCliente, "******************************************************************************" );
	log_info(logCliente, "****************** FUSE: llamada a osada_read() ******************************" );
	log_info(logCliente, "******************************************************************************" );

	log_info(logCliente, path);
	log_info(logCliente, "Size: %d", size);
	//Si path es igual a "/" nos estan pidiendo los atributos del punto de montaje
	if ((strcmp(path, "/.Trash") != 0) && (strcmp(path, "/.Trash-1000") != 0) && (strcmp(path, " /.xdg-volume-info") != 0) && (strcmp(path, "/autorun.inf") != 0) && (strcmp(path, "/.xdg-volume-info") != 0))
	{
		int head = 0;
		t_readbuf *pedido = malloc(sizeof(t_readbuf));
		pedido->pathLen = strlen(path) + 1;
		pedido->size = size;
		pedido->offset = offset;

		char* paquete = NULL;

		pthread_mutex_lock(&mutex_comunicacion);
			enviarConProtocolo(*socketServidor, PEDIDO_READ, pedido);
		    enviarEstructuraRead(*socketServidor, PEDIDO_READ, path, pedido);
			log_info(logCliente, "	Envie PEDIDO_READ");
			paquete =(char*) recibirConProtocolo(*socketServidor,&head);
		pthread_mutex_unlock(&mutex_comunicacion);
		//free(pedido);

		if (head == RESPUESTA_READ)
		{
			memset(buf, 0, size);
			memcpy(buf, paquete, strlen((char*)paquete) + 1);

			log_info(logCliente, "	Recibi RESPUESTA_READ");
		}
		else if (head == ENOENTRY)
		{
			log_info(logCliente, "	Recibi respuesta ENOENT en osada_read....... ........ ..... ...... ..... .... .....");
			//return -ENOENT; //ver este caso si igual devuelvo size o no
		}
		else
		{
			log_info(logCliente, "	No recibi RESPUESTA_READ en osada_read.... ....... ........ ....... ........ ..... ....");
		}
		free(paquete);
	}

	return size;
}

static int osada_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	// escribir un archivo
	log_info(logCliente, "******************************************************************************" );
	log_info(logCliente, "****************** FUSE: llamada a osada_write() *****************************" );
	log_info(logCliente, "******************************************************************************" );

	return 0;
}

static int osada_mkdir(const char *path, mode_t mode)
{
	// crear un directorio
	log_info(logCliente, "******************************************************************************" );
	log_info(logCliente, "****************** FUSE: llamada a osada_mkdir() *****************************" );
	log_info(logCliente, "******************************************************************************" );

//	int head;
//	void *mensaje = NULL;
//
//	enviarConProtocolo(socketServidor,PEDIDO_MKDIR, path);
//	//recibir por socket un OK?
//	mensaje = recibirConProtocolo(socketServidor,&head);
//
//	if (head == RESPUESTA_MKDIR)
//	{
//		//creo que no necesito recibir nada util, que a fuse no le importa nada
//	}
	return 0;
}

static int osada_rmdir(const char *path)
{
	// borrar un directorio
	log_info(logCliente, "******************************************************************************" );
	log_info(logCliente, "****************** FUSE: llamada a osada_mkdir() *****************************" );
	log_info(logCliente, "******************************************************************************" );

//	int head;
//	void *mensaje = NULL;
//
//	enviarConProtocolo(socketServidor,PEDIDO_RMDIR, path);
//	//recibir por socket un OK?
//	mensaje = recibirConProtocolo(socketServidor,&head);
//
//	if (head == RESPUESTA_RMDIR)
//	{
//		//creo que no necesito recibir nada util, que a fuse no le importa nada
//	}
	return 0;
}

static int osada_rename(const char *path, const char *newpath)
{
	// renombrar un archivo
	log_info(logCliente, "******************************************************************************" );
	log_info(logCliente, "****************** FUSE: llamada a osada_rename() ****************************" );
	log_info(logCliente, "******************************************************************************" );

//	int head;
//	void *mensaje = NULL;
//	char *pedido = malloc(sizeof(path) + sizeof(newpath));
//	//ver esto mejor
//	memset(&pedido,0,(sizeof(path) + sizeof(newpath)));
//	memcpy(&pedido, path, sizeof(path));
//	memcpy(&pedido + sizeof(path), newpath, sizeof(newpath));
//
//	enviarConProtocolo(socketServidor,PEDIDO_RENAME, pedido);
//	//recibir por socket un OK?
//	mensaje = recibirConProtocolo(socketServidor,&head);
//
//	if (head == RESPUESTA_RENAME)
//	{
//		//creo que no necesito recibir nada util, que a fuse no le importa nada
//	}
	return 0;
}

static int osada_unlink(const char *path)
{
	// borrar un archivo
	log_info(logCliente, "******************************************************************************" );
	log_info(logCliente, "****************** FUSE: llamada a osada_unlink() ****************************" );
	log_info(logCliente, "******************************************************************************" );

//	int head;
//	void *mensaje = NULL;
//
//	enviarConProtocolo(socketServidor,PEDIDO_UNLINK, path);
//	//recibir por socket un OK?
//	mensaje = recibirConProtocolo(socketServidor,&head);
//
//	if (head == RESPUESTA_UNLINK)
//	{
//		//creo que no necesito recibir nada util, que a fuse no le importa nada
//	}
	return 0;
}

static int osada_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	// crear y abrir un archivo
	log_info(logCliente, "******************************************************************************" );
	log_info(logCliente, "****************** FUSE: llamada a osada_create() ****************************" );
	log_info(logCliente, "******************************************************************************" );

//	int head;
//	void *mensaje = NULL;
//
//	enviarConProtocolo(socketServidor,PEDIDO_CREATE, path);
//	//recibir por socket un OK?
//	mensaje = recibirConProtocolo(socketServidor,&head);
//
//	if (head == RESPUESTA_CREATE)
//	{
//		//creo que no necesito recibir nada util, que a fuse no le importa nada
//	}
	return 0;
}

void printConectado()
{
	printf("\t Recibi mensaje handshake  del servidor\n\n");

	log_info(logCliente, "	Recibi mensaje handshake  del servidor");
	printf( "****************** " GRN "Se establecio la conexion con exito " RESET "***************************\n");
	log_info(logCliente, "****************** Se establecio la conexion con exito *********************" );
}

void printEncabezado()
{
	printf(GRN "\n\n\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" RESET);
	printf("**********************************************************************************\n");
	printf("****************** " GRN "CLIENTE POKEDEX " RESET "***********************************************\n");
	printf("**********************************************************************************\n\n\n");
	printf("****************** Iniciando cliente...\n\n");
	printf("****************** Creando archivo Log *******************************************\n");
}

void printErrorConexion()
{
	printf(RED "\n\n\n****************** No se pudo establecer la conexion con el servidor *************\n" RESET);
	printf("****************** Terminando el programa ****************************************\n");
	printf("**********************************************************************************\n\n\n");
	log_info(logCliente, "****************** No se pudo establecer la conexion con el servidor *************" );
	log_info(logCliente, "****************** Terminando el programa ****************************************" );
	log_info(logCliente, "**********************************************************************************" );

}

void PrintFuse()
{
	printf( "****************** " GRN "Llamo a fuse main" RESET "  ********************************************\n");
	log_info(logCliente, "****************** Llamo a fuse main  ********************************************" );
	printf("**********************************************************************************\n\n\n");
}

void printMensajeInesperado(int head)
{
	printf(YEL "\t Recibi mensaje: %d  del servidor\n\n" RESET, head);
	log_info(logCliente, "	Recibi mensaje inesperado  del servidor");
}

void printServidorDesconectado()
{
	printf(RED "\n\n\n****************** El servidor cerro la conexion *********************************\n" RESET);
	printf("****************** Terminando el programa ****************************************\n");
	printf("**********************************************************************************\n\n\n");
	log_info(logCliente, "****************** No se pudo establecer la conexion con el servidor *************" );
	log_info(logCliente, "****************** Terminando el programa ****************************************" );
	log_info(logCliente, "**********************************************************************************" );
}

void terminar()
{
	printf("Señal terminar");
	exit(0);
}
