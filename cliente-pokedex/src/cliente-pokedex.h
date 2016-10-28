/*
 * cliente-pokedex.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef CLIENTE_POKEDEX_H_
#define CLIENTE_POKEDEX_H_

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

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
#include <shared_configs.h>
#include <shared_sockets.h>
#include <shared_serializacion.h>


//-----Protocolo de mensajes
#define PEDIDO_GETATTR 12
#define PEDIDO_READDIR 13
#define PEDIDO_OPEN 14
#define PEDIDO_READ 15
#define PEDIDO_WRITE 16
#define PEDIDO_UNLINK 17
#define PEDIDO_MKDIR 18
#define PEDIDO_RMDIR 19
#define PEDIDO_RENAME 20
#define PEDIDO_CREATE 21

#define RESPUESTA_GETATTR 22
#define RESPUESTA_READDIR 23
#define RESPUESTA_OPEN 24
#define RESPUESTA_READ 25
#define RESPUESTA_WRITE 26
#define RESPUESTA_UNLINK 27
#define RESPUESTA_MKDIR 28
#define RESPUESTA_RMDIR 29
#define RESPUESTA_RENAME 30
#define RESPUESTA_CREATE 31
#define ENOENTRY 32
//--

#define TOTAL_ARGS 3
#define DEFAULT_FILE_PATH "/"
#define NOMBRE_LOG	"ClientePokedex.log"
#define NOMBRE_PROG	"cliente-pokedex"


//------FUSE

struct t_runtime_options
{
	char* welcome_msg;
} runtime_options;

enum {
KEY_VERSION,
KEY_HELP,
};

static struct fuse_opt fuse_options[] = {
		// Este es un parametro definido por nosotros
		CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),
	// Estos son parametros por defecto que ya tiene FUSE
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_END,
};

//-------Operaciones
static int osada_create(const char *path, mode_t mode, struct fuse_file_info *fi);
  int osada_getattr(const char *path, struct stat *stbuf);
static int osada_mkdir(const char *path, mode_t mode);
  int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int osada_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int osada_rename(const char *path, const char *newpath);
static int osada_rmdir(const char *path);
static int osada_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int osada_unlink(const char *path);

//------

char * ip_pokedex;
int puerto_pokedex;
char * directorio_montaje;
int  socketServidor;

struct stat fileStat;
pthread_t thread_Planificador;
pthread_mutex_t mutex_comunicacion;
t_log* logCliente;

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"


int validar(int argc, char **argv);
int  conectar_con_servidor_pkdx();
void* enviarOperacionAServidor(int operacion, void* buffer_out);
int set_datos_conexion();
void printConectado();
void printEncabezado();
void printErrorConexion();
void PrintFuse();
void printMensajeInesperado(int head);
void printServidorDesconectado();

void terminar();


//Mapeo operaciones Fuse

static struct fuse_operations osada_oper = {
.getattr = osada_getattr,	// obtener atributos
.readdir = osada_readdir, // leer un directorio
//.read = osada_read, // leer archivo
//.write = osada_write, // escribir un archivo
//.unlink = osada_unlink, // borrar un archivo
//.mkdir = osada_mkdir,	// crear un directorio
//.rmdir = osada_rmdir,	//borrar un directorio
//.rename = osada_rename,	//renombrar un archivo
//.create = osada_create //crear y abrir un archivo
};


#endif /* CLIENTE_POKEDEX_H_ */
