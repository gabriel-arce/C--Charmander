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
#include "shared_comunicaciones.h"

//-----Protocolo de mensajes
#define _tomarAtributos 10
#define _leerDirectorio 11
#define _abrir 12
#define _leer 13
#define _limpiar 14
#define _borrarArchivo 15
#define _renombrar 16
#define _cambiarTamanio 17
#define _escribir 18
#define _borrarDirectorio 19
#define _crearArchivo 20
#define _crearDirectorio 21
//--

#define TOTAL_ARGS 2
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
void* enviarOperacionAServidor(int operacion, void* buffer_out);

//-------Operaciones
static int tomar_atributos ( const char *path, struct stat *stbuf );
static int leer_directorio ( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi );
static int abrir ( const char *path, struct fuse_file_info *fi );
static int leer ( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi );
static void limpiar ( void *datos );
static int borrar_archivo ( const char *path );
static int renombrar ( const char *viejo, const char *nuevo );
static int cambiar_tamano ( const char *path, off_t tamanio );
static int escribir ( const char *path, const char *buf, size_t tamanio, off_t offset, struct fuse_file_info *nada );
static int borrar_directorio ( const char *path );
static int crear_archivo(const char *path, mode_t modo, struct fuse_file_info *fi);
static int crear_directorio( const char *path, mode_t modo);


//Mapeo operaciones Fuse

static struct fuse_operations operaciones_fuse = {
			.getattr		= tomar_atributos,
			.readdir		= leer_directorio,
			.open			= abrir,
			.read			= leer,
			.destroy		= limpiar,
			.unlink			= borrar_archivo,
			.rename	 		= renombrar,
			.truncate		= cambiar_tamano,
			.write    		= escribir,
			.rmdir			= borrar_directorio,
			.mkdir			= crear_directorio,
			.create			= crear_archivo,
};

#endif /* CLIENTE_POKEDEX_H_ */
