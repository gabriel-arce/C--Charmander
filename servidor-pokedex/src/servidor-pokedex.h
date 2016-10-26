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
#include <commons/collections/list.h>
#include <shared_sockets.h>
#include <shared_comunicaciones.h>
#include <shared_semaforos.h>
#include <shared_serializacion.h>
#include <sys/socket.h>
#include <sys/types.h>
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
#include "osada.h"
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>



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
#define NOMBRE_LOG	"ServerPokedex.log"
#define NOMBRE_PROG	"servidor-pokedex"
#define PUERTO	5000
#define IP "127.0.0.1"

t_log logServidor;
int	listenningSocket;
pthread_mutex_t mutex_comunicacion;


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


//funciones de disco
void asignarOffsets();
void descargar(uint32_t descriptorArchivo);
void escribirBloque(uint32_t bloque, char* buf);
void escribirArchivo(uint32_t posicion, char* buf);
int existePath(char* path, uint16_t** pos);
int existeDirectorio(unsigned char* token, uint16_t* padre, int* posicion);
void inicializarDisco();
void leerArchivo(uint32_t posicion, osada_file* buf);
void leerAsignacion(uint32_t posicion, osada_file* buf);
void leerBloque(uint32_t cantidadBloques, char* buf);
void leerDato(uint32_t posicion, osada_file* buf);
void leerHeader();
int mapearDisco(char* path);
void mostrarHeader(osada_header oheader);
void* readdir(char* path);
void leerTablaArchivos();
void leerTablaAsignaciones();
void leerTablaDatos();
//estas no se si funcionan
void* readFile(osada_file ofile, void *buffer);
void* readData(int cant_blocks, int* fat_values, void *buffer);
void* concatenate(void *buffer, int tamBuffer, void *tmpBuffer, int tamTmpBuffer, void* result);


//funciones de servidor-------------------------------------------------
void atendercliente(int socket);
void* hiloComunicacion(void* arg);
void printEncabezado();
void printTerminar();
void procesarPedidoCreate(void *pedido, void *respuesta);
void* procesarPedidoGetatrr(char *path);
void procesarPedidoMkdir(void *pedido, void *respuesta);
void procesarPedidoOpen(void *pedido, void *respuesta);
void* procesarPedidoRead(void *buffer);
void* procesarPedidoReaddir(char *path);
void procesarPedidoRename(void *pedido, void *respuesta);
void procesarPedidoRmdir(void *pedido, void *respuesta);
void procesarPedidoUnlink(void *pedido, void *respuesta);
void procesarPedidoWrite(void *pedido, void *respuesta);
void terminar();

#endif /* SERVIDOR_POKEDEX_H_ */
