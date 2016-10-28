#ifndef COMUNICACION_H_
#define COMUNICACION_H_

#include <fuse.h>
#include <stdint.h>
#include <sys/stat.h>

#define HANDSHAKE  777
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

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

//-----estructuras para paquetes-------------------------------------------------------------------------------------

//para el stat de fuse en getattr
typedef struct
{
	mode_t  mode;
	nlink_t  nlink;
	off_t  size;
}__attribute__((packed))
t_stbuf;

//para fuse en pedido write
typedef struct
{
	size_t size;
	off_t offset;
	int pathLen;
	int bufLen;

}__attribute__((packed))
t_writebuf;

//para   fuse en pedido read
typedef struct
{
	size_t size;
	off_t offset;
	int pathLen;
}__attribute__((packed))
t_readbuf;

//para el  fuse en respuesta read
typedef struct
{
	char *buf; 
}__attribute__((packed))
t_readRespuesta;

//header para enviar mensajes
typedef struct {
char tipo;
int tamanio;
}__attribute__((packed)) t_header;

int recibirInt(int socket);
void enviarInt32(uint32_t mensaje, int socket) ;
void enviarInt(int mensaje, int socket);
int aceptarConexion(int listenningSocket);
int crearServer(char * puerto);
int recibirPorSocket(int skServidor, void * buffer, int tamanioBytes);
int enviarPorSocket(int fdCliente, const void * mensaje, int tamanioBytes);
int calcularTamanioMensaje(int head, void* mensaje);
void * serializar(int head, void * mensaje, int tamanio);
void * deserializar(int head, void * buffer, int tamanio);
int enviarConProtocolo(int fdReceptor, int head, void *mensaje);
void* recibirConProtocolo(int socketEmisor,int* head);
int crearSocket(char ip[], char puerto[]);

void* serializarPedidoGetatrr(t_stbuf* response, int tamanio);
void* serializarPedidoRead(t_readbuf* response, char* path);
void* recibirEstructuraRead(int socketEmisor,int* head);
int enviarEstructuraRead(int fdReceptor, int head, char* path, t_readbuf* mensaje);
void* recibirEstructuraWrite(int socketEmisor,int* head);
void* serializarPedidoWrite(t_writebuf* response, char* path, char* bufWrite);
int enviarEstructuraWrite(int fdReceptor, int head, char* path, char* bufWrite, t_writebuf* mensaje);
#endif
