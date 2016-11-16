/*
 * pokedexCliente.h

 *
 *  Created on: 5/10/2016
 *      Author: guadalupe
 */



#ifndef POKEDEXCLIENTE_H_
#define POKEDEXCLIENTE_H_

#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <comunicacion.h>

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

//pasar a un .h los pedidos respuestas
#define HANDSHAKE 777



//Variables Globales
int* socketServidor;
t_log* logCliente;

char* puntoDeMontaje;
char mensaje[6] = "\0";
//char* ip;
//char* puerto;

char ip[10];
char puerto[5];

int iret1;
pthread_t thread_Planificador;
pthread_mutex_t mutex_comunicacion  = PTHREAD_MUTEX_INITIALIZER;

struct t_runtime_options
{
	char* welcome_msg;
} runtime_options;

static struct fuse_operations osada_oper;

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

struct tm *timeinfo;



//Funciones
static int osada_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int osada_getattr(const char *path, struct stat *stbuf);
static int osada_flush(const char *path, struct fuse_file_info *fi);
static int osada_mkdir(const char *path, mode_t mode);
static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int osada_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int osada_release(const char *path, struct fuse_file_info *fi);
static int osada_rename(const char *path, const char *newpath);
static int osada_rmdir(const char *path);
static int osada_truncate(const char *path, off_t new_size);
static int osada_unlink(const char *path);
static int osada_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int osada_open(const char *path, struct fuse_file_info *fi);
static int osada_access(const char *filename, int how);


void printConectado();
void printEncabezado();
void printErrorConexion();
void PrintFuse();
void printMensajeInesperado(int head);
void printServidorDesconectado();

void terminar();

time_t obtenerFecha();


#endif /* POKEDEXCLIENTE_H_ */
