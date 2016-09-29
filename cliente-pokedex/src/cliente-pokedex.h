/*
 * cliente-pokedex.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef CLIENTE_POKEDEX_H_
#define CLIENTE_POKEDEX_H_

//#define FUSE_USE_VERSION 27
#define _FILE_OFFSET_BITS		64
#define FUSE_USE_VERSION		30

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <commons/string.h>
#include <shared_configs.h>
#include <shared_sockets.h>
#include <fuse.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <commons/log.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define TOTAL_ARGS 2
#define PORT 9000
#define IP "127.0.0.1"

#define DEFAULT_FILE_PATH "/"

#define NOMBRE_LOG	"ClientePokedex.log"
#define NOMBRE_PROG	"cliente-pokedex"

typedef enum{
	CantidadArgumentosIncorrecta,
	NoSePudoAbrirIn,
	ErrorEnLectura,
	ErrorEnEscritura,
	ErrorStat,
	ErrorMmap,
	OtroError,
} Error;

typedef struct {
	char *nombre;											//Nombre del archivo
	char *path;													//Nombre real
	struct stat info;										//Estructura stat del archivo
	struct listaArchivos *siguiente;		//Siguiente nodo
} t_listaArchivos;




t_listaArchivos *ListaArchivos;

static struct fuse_operations operaciones;

char * ip_pokedex;
int puerto_pokedex;
char * directorio_montaje;
int socket_pokedex;

FILE *in;
struct stat fileStat;
t_log *logger;
char *hdr;


void mostrarAyuda();
void mostrarMensajeDeError( Error );
void crear_logger();
void cerrarDisco();

int validar(int argc, char **argv);

int conectar_con_servidor_pkdx();

static int tomar_atributos ( const char *path, struct stat *stbuf );
static int leer_directorio ( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi );
static int abrir ( const char *path, struct fuse_file_info *fi );
static int leer ( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi );
static void limpiar ( void *datos );
static int borrar_archivo ( const char *path );
static int renombrar ( const char *viejo, const char *nuevo );
static int crear_nodo ( const char *path, mode_t modo, dev_t dispositivo );
static int cambiar_tamano ( const char *path, off_t tamanio );
static int escribir ( const char *path, const char *buffer, size_t tamanio, off_t offset, struct fuse_file_info *nada );
static int borrar_directorio ( const char *path );
static int crear_archivo(const char *path, mode_t modo, struct fuse_file_info *fi);
static int crear_directorio( const char *path, mode_t modo);
static int tomar_atributos_extendidos(const char *path, const char *nombre, char *valor, size_t tamanio);
static int verificar_acceso(const char *path, int mask);

#endif /* CLIENTE_POKEDEX_H_ */
