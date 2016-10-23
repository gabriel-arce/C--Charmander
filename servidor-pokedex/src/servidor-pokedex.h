/*
 * servidor-pokedex.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef SERVIDOR_POKEDEX_H_
#define SERVIDOR_POKEDEX_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <commons/log.h>
#include <signal.h>
#include <pthread.h>
#include <shared_sockets.h>
#include <shared_comunicaciones.h>
#include <shared_semaforos.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <commons/collections/list.h>
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

#define NOMBRE_LOG	"ServerPokedex.log"
#define NOMBRE_PROG	"servidor-pokedex"
#define PUERTO	5000
#define IP "127.0.0.1"


#define BLOCK_SIZE				64
#define FILETABLE				1024
#define OSADA_FILENAME_LENGTH	17
#define MAX_FILES				2048
#define HEADER_BLOCK			1

//Estructuras OSADA

typedef uint32_t osada_block_pointer;

typedef struct{
	unsigned char identificador[7];
	uint8_t	version;
	uint32_t bloquesFS;
	uint32_t bloquesBitMap;
	uint32_t inicioTablaAsignacion;
	uint32_t bloquesDatos;
	unsigned char relleno[40];
} t_osada_hdr;

typedef enum __attribute__((packed)) {
    DELETED = '\0',
    REGULAR = '\1',
    DIRECTORY = '\2',
} osada_file_state;

typedef struct {
	osada_file_state state;
	unsigned char fname[OSADA_FILENAME_LENGTH];
	uint16_t parent_directory;
	uint32_t file_size;
	uint32_t lastmod;
	osada_block_pointer first_block;
} t_osada_file;

//Sesion

typedef struct {
	int socket_cliente;
	char * directorio_montaje;
} t_sesion_cliente;

t_log *logger;
pthread_t pIDServer;
int fd_servidor;
pthread_mutex_t mutex_servidor;
bool server_on;
t_list * clientes_conectados;

void validar();
void inicializarVariables();
void crearLog();
void crearSemaforos();
void destruirSemaforos();
void crearServer();
t_sesion_cliente * crearSesionCliente(int cli_socket);
void * serverCliente(void * args);


#endif /* SERVIDOR_POKEDEX_H_ */
