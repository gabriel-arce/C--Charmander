#ifndef COMUNICACION_H_
#define COMUNICACION_H_

#include <commons/config.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/bitarray.h>

#include <fuse.h>
#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <math.h>
#include <sys/mman.h>
#include <signal.h>
#include <stddef.h>


#define HANDSHAKE 777
#define ERROR -1

#define PEDIDO_GETATTR 12
#define PEDIDO_READDIR 13
#define PEDIDO_TRUNCATE 14
#define PEDIDO_READ 15
#define PEDIDO_WRITE 16
#define PEDIDO_UNLINK 17
#define PEDIDO_MKDIR 18
#define PEDIDO_RMDIR 19
#define PEDIDO_RENAME 20
#define PEDIDO_CREATE 21

#define RESPUESTA_GETATTR 22
#define RESPUESTA_READDIR 23
#define RESPUESTA_TRUNCATE 24
#define RESPUESTA_READ 25
#define RESPUESTA_WRITE 26
#define RESPUESTA_UNLINK 27
#define RESPUESTA_MKDIR 28
#define RESPUESTA_RMDIR 29
#define RESPUESTA_RENAME 30
#define RESPUESTA_CREATE 31
#define ENOENTRY 32

#define PEDIDO_OPEN 33
#define RESPUESTA_OPEN 34
#define PEDIDO_RELEASE 35
#define RESPUESTA_RELEASE 36
#define PEDIDO_TRUNCATE_NEW_SIZE 37
#define PEDIDO_FLUSH 38
#define RESPUESTA_FLUSH 39

#define PEDIDO_UTIMENS 40
#define RESPUESTA_UTIMENS 41
#define RESPUESTA_ERROR 42

#define ERRDQUOT 43 //archivo 2049, no hay espacio en la tabla de archivos
#define ERRFBIG 44 //no hay bloques de datos disponibles
#define ERRNAMETOOLONG 45 //nombres de archivos con mas de 17 caracteres
#define PEDIDO_MKNOD 46
#define RESPUESTA_MKNOD 47
#define ERRNOSPC 48

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"
#define NAR "\033[38;5;166m"//naranja
#define PINK "\033[38;5;168m"//rosa
#define VIO "\033[38;5;129m"//violeta
#define PINK2 "\033[38;5;207m"//violeta
#define ORG "\033[38;5;214m"//naranja
#define AMB "\033[38;5;100m"//verde
#define YEL2 "\033[38;5;226m"//verde
#define RED2 "\033[38;5;196m"//ROJO
#define COR "\033[1;31m"
//-----estructuras para paquetes-------------------------------------------------------------------------------------

//para el stat de fuse en getattr
typedef struct{
	mode_t  mode;
	nlink_t  nlink;
	off_t  size;
	//time_t mtime;
}__attribute__((packed)) t_stbuf;

//para fuse en pedido write
typedef struct{
	size_t size;
	off_t offset;
	int pathLen;
	int bufLen;
}__attribute__((packed)) t_writebuf;

//para   fuse en pedido read
typedef struct{
	size_t size;
	off_t offset;
	int pathLen;
}__attribute__((packed)) t_readbuf;

//para el  fuse en respuesta read
typedef struct{
	char *buf;
}__attribute__((packed)) t_readRespuesta;

//header para enviar mensajes
typedef struct {
	char tipo;
	int tamanio;
}__attribute__((packed)) t_header;


void* recibirRespuestaRead(int socketEmisor, int* head, uint32_t* tamanio);
int enviarRespuestaRead(int socket, int head, void* respuesta, uint32_t* tamanioBuffer);

int aceptarConexion(int listenningSocket);
int crearServer(char * puerto);
int recibirPorSocket(int skServidor, void * buffer, int tamanioBytes);
int enviarPorSocket(int fdCliente, void* mensaje, int tamanioBytes);
int calcularTamanioMensaje(int head, void* mensaje);
void * serializar(int head, void * mensaje, int tamanio);
void * deserializar(int head, void * buffer, int tamanio);
int enviar(int fdReceptor, int head, void *mensaje);
void* recibir(int socketEmisor,int* head);
int crearSocket(char ip[], char puerto[]);

void* serializarPedidoGetatrr(t_stbuf* response, int tamanio);
void* serializarPedidoRead(t_readbuf* response, char* path);
void* recibirEstructuraRead(int socketEmisor,int* head);
int enviarEstructuraRead(int fdReceptor, int head, char* path, t_readbuf* mensaje);
void* recibirEstructuraWrite(int socketEmisor,int* head);
void* serializarPedidoWrite(t_writebuf* response, char* path, char* bufWrite);
int enviarEstructuraWrite(int fdReceptor, int head, char* path, char* bufWrite, t_writebuf* mensaje);
#endif
