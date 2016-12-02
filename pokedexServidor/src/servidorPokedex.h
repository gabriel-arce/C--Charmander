/*
 * servidorPokedex.h
 *
 *  Created on: 5/10/2016
 *      Author: utnso
 */
#include <commons/collections/queue.h>
#include <comunicacion.h>
#include <osada.h>
#include <signal.h>
#include <pthread.h>

#ifndef SERVIDORPOKEDEX_H_
#define SERVIDORPOKEDEX_H_

#define PUERTO "4969"
//#define BACKLOG 15	// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
#define ERRNOSPC 48

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
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

#define BLOCK_SIZE	64
#define MAX_THREADS 5

t_log logServidor;
int	listenningSocket;

void* atendercliente(void* socketCliente);
void destruirMutex() ;
void* hiloComunicacion(void* arg);
void printEncabezado();
void printTerminar();

void* procesarCrearEntradaTablaDeArchivos(char *path, int* codigo, int modo);

void* procesarPedidoCreate(char *path, int* codigo);
void* procesarPedidoGetatrr(char *path);
void* procesarPedidoFlush(char *path);
void* procesarPedidoMkdir(char *path, int* codigo);
void* procesarPedidoMknod(char *path, int* codigo);
void* procesarPedidoOpen(char* path, int* codigo);
void* procesarPedidoRead(void* buffer, uint32_t* tamanioBuffer);
void* procesarPedidoReaddir(char *path);
void* procesarPedidoRelease(char* path);
void* procesarPedidoRename(char *paths);
void* procesarPedidoRmdir(char *path);
void* procesarPedidoTruncate(off_t newSize, char* path);
void* procesarPedidoUnlink(char *path);
void* procesarPedidoUtimens(char *path);
void* procesarPedidoWrite(void *buffer, int* codigo);

void mostrar_lista_archivos();
char verificar_permiso_archivo(char *path);

void terminar();
//void threadsDestroyer(pthread_t* thread);

#endif /* SERVIDORPOKEDEX_H_ */
