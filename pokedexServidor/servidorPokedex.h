/*
 * servidorPokedex.h
 *
 *  Created on: 5/10/2016
 *      Author: utnso
 */

#ifndef SERVIDORPOKEDEX_H_
#define SERVIDORPOKEDEX_H_

#define PUERTO "4969"
#define BACKLOG 15	// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
//pasar a un .h los pedidos respuestas
#define HANDSHAKE 777

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

void atendercliente(int socket);
void* hiloComunicacion(void* arg);
void liberarRecursos(); //TODO
void printEncabezado();
void printTerminar();

void* procesarPedidoCreate(char *pedido);
void* procesarPedidoGetatrr(char *path);
void* procesarPedidoMkdir(char *path);
void* procesarPedidoOpen(char* path);
void* procesarPedidoRead(void* buffer, uint32_t* tamanioBuffer);
void* procesarPedidoReaddir(char *path);
void* procesarPedidoRelease(char* path);
void* procesarPedidoRename(char *paths);
void* procesarPedidoRmdir(char *path);
void* procesarPedidoTruncate(off_t newSize, char* path);
void* procesarPedidoUnlink(char *path);
void* procesarPedidoWrite(void *buffer);

void terminar();

#endif /* SERVIDORPOKEDEX_H_ */
