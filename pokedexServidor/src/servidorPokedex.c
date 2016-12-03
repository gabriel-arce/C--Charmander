/*
* servidorPokedex.c
*
*  Created on: 29/9/2016
*      Author: Guadalupe
*/

#include "servidorPokedex.h"


t_queue* threadQueue;
pthread_t thread1;
sem_t semThreads;

int main(int argc, char ** argv)
{
    threadQueue = queue_create();

	printEncabezado();
	inicializarDisco();

	sem_init(&semThreads, 0, MAX_THREADS);
	listenningSocket = crearServer(PUERTO);

	pthread_create(&thread1, NULL, hiloComunicacion, NULL);

	signal(SIGINT,terminar);

	pthread_join(thread1, NULL);
	pthread_detach(thread1);
	close(listenningSocket);
	descargar();

	printTerminar();
	return(0);
}

void* hiloComunicacion(void* arg)
{
	int head;
	void *mensaje = NULL;

	while(1)
	{
		sem_wait(&semThreads); //espera que se libere algun thread para conectar otro cliente, tiene un maximo

		int* socketCliente = malloc(sizeof(int));
		*socketCliente = aceptarConexion(listenningSocket);

		mensaje = recibir(*socketCliente,&head);

		char* mensajeHSK = mensaje;

		if (mensajeHSK)
		{
			int enviado = enviar(*socketCliente, HANDSHAKE, mensajeHSK);
			if (enviado == -1)
			{
				printf(YEL "\t El cliente %d se desconecto \n " RESET, *socketCliente);
			}
			else
			{

				pthread_attr_t attr;
				pthread_t* cliente = malloc(sizeof(pthread_t));
				queue_push(threadQueue, cliente);

				pthread_attr_init(&attr);
				pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
				cliente = queue_pop(threadQueue);
				pthread_create(cliente, &attr, atendercliente, (void*)socketCliente);
				pthread_attr_destroy(&attr);
				free(cliente);
			}
		}
		else
		{
			//printf(YEL "\t Recibi un mensaje inesperado de: %d :%s \n" RESET, *socketCliente, mensajeHSK);
		}
	}
	pthread_exit((void*) "Finaliza hilo comunicacion");
	return NULL;
}

void* atendercliente(void* socketCliente)
{
	int continuar = 1;
	int codigo = 0;
	int* sock = socketCliente;
	int socket =*sock;
	void *respuesta = NULL;
	void *pedido = NULL;
	free(sock);

	while(continuar)
	{
		int head = 0;
		pedido = recibir(socket, &head);
		if(pedido != NULL)
		{
			switch(head)
			{
				case PEDIDO_CREATE:

					printf(MAG "\t procesando PEDIDO_CREATE de %d\n" RESET, socket);
					codigo = RESPUESTA_CREATE;
					respuesta = procesarCrearEntradaTablaDeArchivos((char*)pedido, &codigo, 1);

					switch(codigo)
					{
						case RESPUESTA_CREATE:

							enviar(socket, RESPUESTA_CREATE, respuesta);
							printf(MAG "\t devolviendo RESPUESTA_CREATE a %d\n" RESET, socket);
							break;

						case ERRDQUOT:

							enviar(socket, ERRDQUOT, respuesta);
							printf(YEL "\t devolviendo ERRDQUOT a %d\n" RESET, socket);
							break;

						case ERRNAMETOOLONG:

							enviar(socket, ERRNAMETOOLONG, respuesta);
							printf(YEL "\t devolviendo ERRNAMETOOLONG a %d\n" RESET, socket);
							break;

						case RESPUESTA_ERROR:

							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(MAG "\t respondiendo  a %d\n" RESET, socket);
							break;

					    default:

							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(PINK2 "\t respondiendo  a %d\n" RESET, socket);
							break;
					}

					free(pedido);
					pedido = NULL;
					break;

				case PEDIDO_GETATTR:
					printf( "\t procesando PEDIDO_GETATTR de %d\n", socket);
					respuesta = procesarPedidoGetatrr((char*)pedido);

					free(pedido);
					pedido = NULL;

					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_GETATTR, respuesta);
						printf( "\t devolviendo RESPUESTA_GETATTR a %d\n" RESET, socket);
					}
					else
					{
						int* retorno = malloc(sizeof(int));
						*retorno = ENOENTRY;
						enviar(socket, ENOENTRY, retorno);
						printf("\t devolviendo respuesta ENOENT a %d\n" RESET, socket);
					}
					break;

				case PEDIDO_FLUSH:
					printf(GRN "\t procesando PEDIDO_FLUSH de %d\n" RESET, socket);
					respuesta = procesarPedidoFlush((char*)pedido);

					free(pedido);
					pedido = NULL;
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_FLUSH, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_FLUSH a %d\n" RESET, socket);
					}
					else
					{
						int* retorno = malloc(sizeof(int));
						*retorno = ENOENTRY;

						enviar(socket, ENOENTRY, retorno);
						printf(YEL "\t devolviendo respuesta ENOENT a %d\n" RESET, socket);
					}
					break;

				case PEDIDO_MKDIR:
					printf(MAG "\t procesando PEDIDO_MKDIR de %d\n" RESET, socket);

				    codigo = RESPUESTA_MKDIR;
					respuesta = procesarCrearEntradaTablaDeArchivos((char*)pedido, &codigo, 2);

				//	printf(MAG "\t codigo:%d\n" RESET, codigo);
					switch(codigo)
					{
						case RESPUESTA_CREATE:

							enviar(socket, RESPUESTA_MKDIR, respuesta);
							printf(MAG "\t devolviendo RESPUESTA_MKDIR a %d\n" RESET, socket);
							break;

						case ERRDQUOT:

							enviar(socket, ERRDQUOT, respuesta);
							printf(YEL "\t devolviendo ERRDQUOT a %d\n" RESET, socket);
							break;

						case ERRNAMETOOLONG:

							enviar(socket, ERRNAMETOOLONG, respuesta);
							printf(YEL "\t devolviendo ERRNAMETOOLONG a %d\n" RESET, socket);
							break;

						case RESPUESTA_ERROR:

							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(MAG "\t respondiendo  a %d\n" RESET, socket);
							break;

					    default:

							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(MAG "\t respondiendo a %d\n" RESET, socket);
							break;
					}

					free(pedido);
					pedido = NULL;
					break;

				case PEDIDO_MKNOD:
					printf(MAG "\t procesando PEDIDO_MKNOD de %d\n" RESET, socket);
					codigo = RESPUESTA_MKNOD;

					respuesta = procesarCrearEntradaTablaDeArchivos((char*)pedido, &codigo, 1);

					switch(codigo)
					{
						case RESPUESTA_CREATE:

							enviar(socket, RESPUESTA_MKNOD, respuesta);
							printf(MAG "\t devolviendo RESPUESTA_MKNOD a %d\n" RESET, socket);
							break;

						case ERRDQUOT:

							enviar(socket, ERRDQUOT, respuesta);
							printf(YEL "\t devolviendo ERRDQUOT a %d\n" RESET, socket);
							break;

						case ERRNAMETOOLONG:

							enviar(socket, ERRNAMETOOLONG, respuesta);
							printf(YEL "\t devolviendo ERRNAMETOOLONG a %d\n" RESET, socket);
							break;

						case RESPUESTA_ERROR:

							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(MAG "\t respondiendo a %d\n" RESET, socket);
							break;

					    default:

							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(PINK2 "\t respondiendo a %d\n" RESET, socket);
							break;
					}

					free(pedido);
					pedido = NULL;
					break;

				case PEDIDO_OPEN:
					printf(GRN "\t procesando PEDIDO_OPEN de %d\n" RESET, socket);
					codigo = RESPUESTA_OPEN;

					respuesta = procesarPedidoOpen((char*)pedido, &codigo);
					if(codigo == RESPUESTA_OPEN)
					{
						enviar(socket, RESPUESTA_OPEN, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_OPEN a %d\n" RESET, socket);
					}
					else if(codigo == ENOENTRY)
					{
						int* retorno= malloc(sizeof(int));
						*retorno = ENOENTRY;

						enviar(socket, ENOENTRY, retorno);
						printf(YEL "\t devolviendo respuesta ENOENT a %d\n" RESET, socket);
					}
					else//devolver algun codigo que indique que el archivo esta bloqueado
					{
//						enviar(socket, BLOQUEADO, respuesta);//EBUSY?
//						printf(RED "\t devolviendo respuesta BLOQUEADO \n" RESET);
					int* retorno= malloc(sizeof(int));
					*retorno = ENOENTRY;

					enviar(socket, ENOENTRY, retorno);
					printf(YEL "\t devolviendo respuesta ENOENT a %d\n" RESET, socket);
					}
					break;

				case PEDIDO_READ:
					printf( BLU "\t procesando PEDIDO_READ de %d\n" RESET, socket);

					void *buffer = NULL;
					uint32_t* tamanioBuffer = malloc(sizeof(uint32_t));
					memset(tamanioBuffer, 0, sizeof(uint32_t));

					buffer = recibirEstructuraRead(socket, &head);
					respuesta = procesarPedidoRead(buffer, tamanioBuffer);

					free(pedido);
					pedido = NULL;

					if(respuesta != NULL)
					{
						enviarRespuestaRead(socket, RESPUESTA_READ, respuesta, tamanioBuffer);
						printf(BLU "\t devolviendo RESPUESTA_READ a %d\n" RESET, socket);
					}
					else
					{
						free(tamanioBuffer);
						tamanioBuffer = NULL;
						int* retorno = malloc(sizeof(int));
						*retorno = ENOENTRY;

						enviar(socket, ENOENTRY, retorno);
						printf(YEL "\t devolviendo respuesta ENOENT a %d\n" RESET, socket);
					}
					break;

				case PEDIDO_READDIR:
					printf( "\t procesando PEDIDO_READDIR de %d\n", socket);

					respuesta = procesarPedidoReaddir((char*)pedido);

					free(pedido);
					pedido = NULL;

					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_READDIR, respuesta);
						printf( "\t devolviendo RESPUESTA_READDIR a %d\n" RESET, socket);
					}
					else
					{
						int* retorno = malloc(sizeof(int));
						*retorno = ENOENTRY;

						enviar(socket, ENOENTRY, retorno);
						printf("\t devolviendo respuesta ENOENT a %d\n" RESET, socket);
					}
					break;

				case PEDIDO_RELEASE:
					printf(GRN "\t procesando PEDIDO_RELEASE de %d\n" RESET, socket);

					respuesta = procesarPedidoRelease((char*)pedido);
					enviar(socket, RESPUESTA_RELEASE, respuesta);

					printf(GRN "\t devolviendo RESPUESTA_RELEASE a %d\n" RESET, socket);

					break;

				case PEDIDO_RENAME:
					printf(NAR "\t procesando PEDIDO_RENAME de %d\n" RESET, socket);

					respuesta = procesarPedidoRename((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_RENAME, respuesta);
						printf(NAR "\t devolviendo RESPUESTA_RENAME a %d\n" RESET, socket);
					}
					else
					{
						enviar(socket, ERRNAMETOOLONG, pedido);
						printf(YEL "\t devolviendo respuesta ERRNAMETOOLONG a %d\n" RESET, socket);
					}
					break;

				case PEDIDO_RMDIR:
					printf(PINK "\t procesando PEDIDO_RMDIR de %d\n" RESET, socket);
					respuesta = procesarPedidoRmdir((char*)pedido);

					enviar(socket, RESPUESTA_RMDIR, respuesta);
					printf(PINK "\t devolviendo RESPUESTA_RMDIR a %d\n" RESET, socket);

					break;

				case PEDIDO_TRUNCATE:
					printf(NAR "\t procesando PEDIDO_TRUNCATE de %d\n" RESET, socket);

					off_t *newSize = NULL;
					newSize = (off_t*)recibir(socket, &head);

					if (head == PEDIDO_TRUNCATE_NEW_SIZE)
					{
						respuesta = procesarPedidoTruncate(*newSize, (char*)pedido);
					}
					if(respuesta != NULL)
					{
						free(pedido);
						pedido = NULL;

						enviar(socket, RESPUESTA_TRUNCATE, respuesta);
						printf(NAR "\t devolviendo RESPUESTA_TRUNCATE a %d\n" RESET, socket);
					}
					else
					{
						enviar(socket, RESPUESTA_ERROR, pedido);
						printf(YEL "\t devolviendo RESPUESTA_ERROR a %d\n" RESET, socket);
					}

					free(newSize);
					newSize = NULL;
					break;

				case PEDIDO_UNLINK:
					printf(PINK "\t procesando PEDIDO_UNLINK de %d\n" RESET, socket);

					respuesta = procesarPedidoUnlink((char*)pedido);
					enviar(socket, RESPUESTA_UNLINK, respuesta);

					printf(PINK "\t devolviendo RESPUESTA_UNLINK a %d\n" RESET, socket);
					break;

				case PEDIDO_UTIMENS:
					printf(NAR "\t procesando UTIMENS de %d\n" RESET, socket);

					respuesta = procesarPedidoUtimens((char*)pedido);
					enviar(socket, RESPUESTA_UTIMENS, respuesta);

					printf(NAR "\t devolviendo RESPUESTA_UTIMENS a %d\n" RESET, socket);
					break;

				case PEDIDO_WRITE:
					printf( PINK "\t procesando PEDIDO_WRITE de %d\n" RESET, socket);
					codigo = RESPUESTA_WRITE;
					void *bufWrite = NULL;
					int* retorno = NULL;

					bufWrite = recibirEstructuraWrite(socket, &head);

					respuesta = procesarPedidoWrite(bufWrite, &codigo);

					switch(codigo)
					{
						case RESPUESTA_WRITE:

							enviar(socket, RESPUESTA_WRITE, respuesta);
							printf(PINK "\t devolviendo RESPUESTA_WRITE a %d\n" RESET, socket);
							break;

						case ERRFBIG:

							enviar(socket, ERRFBIG, respuesta);
							printf(YEL "\t devolviendo ERRFBIG a %d\n" RESET, socket);
							break;

					    default:

					    	free(respuesta);
					    	respuesta = NULL;
							retorno = malloc(sizeof(int));
							*retorno = ENOENTRY;

							enviar(socket, ENOENTRY, retorno);
							printf(PINK2 "\t respondiendo  a %d\n" RESET, socket);
							break;
					}
					free(pedido);
					pedido = NULL;
					break;

				default:
					printf(RED "\n¿Porqué entre en default???, ¿tenia que enviar un handshake por segunda vez??? \n\n" RESET);

					enviar(socket,HANDSHAKE, pedido);
					break;
			}

		}
		else
		{
			//printf(YEL "\n******** Se desconecto el cliente %d ***************************************\n" RESET, socket);
			continuar = 0;
		}
	}//fin while
	printf(YEL "\n******** Se desconecto el cliente %d, termina el hilo que lo atiendia ******\n" RESET, socket);
	sem_post(&semThreads);
	pthread_exit((void*) "Finaliza hilo cliente");
	return NULL;
}

//funciones de servidor para atender pedidos de cliente--------------------------------------------
void printTerminar()
{
	printf(NAR"**********************************************************************************\n");
	printf(ORG"****************** El servidor cierra la conexion ********************************\n");
	printf(YEL"**********************************************************************************\n");
	printf(YEL2"****************** " GRN "Terminar" YEL2 " ******************************************************\n");
	printf(AMB"**********************************************************************************\n");
	printf(GRN "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n" RESET);
}

void printEncabezado()
{
	printf(GRN "\n\n\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" RESET);
	printf("**********************************************************************************\n");
	printf("**************** POKEDEX SERVIDOR ************************************************\n");
	printf("**********************************************************************************\n\n");

	printf("**************** Iniciando servidor..\n\n");
}

void* procesarCrearEntradaTablaDeArchivos(char *path, int* codigo, int modo)
{
	pthread_rwlock_wrlock(&lockTablaArchivos);
	char r = crearArchivo(path, modo);
	pthread_rwlock_unlock(&lockTablaArchivos);

	char* respuesta = malloc(sizeof(char));
	memset(respuesta, 0, sizeof(char));
	memcpy(respuesta, &r, sizeof(char));

	switch(respuesta[0])
	{
		case 's':
			*codigo = RESPUESTA_CREATE;//esta respuesta la hice funcionar como generica exitosa para create, mkdir, mknod
			break;

		case  'n'://si no hay espacio en la tabla de archivos
			*codigo = ERRDQUOT;
			break;

		case 'x'://nombre de archivo con mas de 17 caracteres
			*codigo = ERRNAMETOOLONG;
			break;

		case 'e'://error
			*codigo = RESPUESTA_ERROR;
			break;
	}

	//printf("	 En crear entrada, devuelvo codigo: %d\n", *codigo );
	return respuesta;
}

void* procesarPedidoGetatrr(char *path)
{
	printf("\t path: %s\n", path);

    if (strcmp(path, "/") == 0)
	{
    	return attrRaiz();
	}
    pthread_rwlock_rdlock(&lockTablaArchivos);
    void* res = getAttr(path);
	pthread_rwlock_unlock(&lockTablaArchivos);
	return res;

}

void* procesarPedidoFlush(char *path)
{
	printf("\t path: %s\n", path);
	char* respuesta = malloc(sizeof(char));

	respuesta[0] = flushArchivo(path);
	return respuesta;
}

void* procesarPedidoOpen(char* path, int* codigo)
{
	printf("\t path: %s\n", path);
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = abrirArchivo(path);

	if(respuesta[0] == 's')
		*codigo = RESPUESTA_OPEN;
	if(respuesta[0] == 'n')
		*codigo = ENOENTRY;
//	else
//		*codigo = BLOQUEADO;

	free(path);
	path = NULL;
	return respuesta;
}

void mostrar_lista_archivos(){
}

void* procesarPedidoRelease(char* path)
{
	printf("\t path: %s\n", path);
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = liberarArchivo(path);
	free(path);
	path = NULL;
	return respuesta;
}

void* procesarPedidoRead(void* buffer, uint32_t* tamanioBuffer)
{
	int desplazamiento = 0;
	int pathLen = 0;
	size_t* size = malloc(sizeof(size_t));
	off_t* offset = malloc(sizeof(off_t));

	memset(size, 0, sizeof(size_t));
	memset(offset, 0, sizeof(off_t));

	memcpy(size, buffer, sizeof(size_t));
	desplazamiento += sizeof(size_t);
	memcpy(offset, buffer + desplazamiento, sizeof(off_t));
	desplazamiento += sizeof(off_t);
	memcpy(&pathLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	char* path = malloc(pathLen);
	memset(path, 0, pathLen);
	memcpy(path,  buffer + desplazamiento, pathLen);

	printf( "\t size: %d bytes\n", *size);
	printf( "\t offset: %d bytes\n", (uint32_t)*offset);
	printf(CYN "\t path: %s\n" RESET, path);

	pthread_rwlock_rdlock(&lockTablaArchivos);
	void* respuesta = readBuffer(path, size, offset, tamanioBuffer);
	pthread_rwlock_unlock(&lockTablaArchivos);

	free(buffer);
	free(path);
	free(size);
	free(offset);
	buffer = NULL;
	path = NULL;
	size = NULL;
	offset = NULL;
	return respuesta;
}

void* procesarPedidoReaddir(char *path)
{
	printf("\t path: %s\n", path);

	pthread_rwlock_rdlock(&lockTablaArchivos);
	void* res = readdir(path);
	pthread_rwlock_unlock(&lockTablaArchivos);

	return res;
}

void* procesarPedidoRename(char *paths)//path - newpath
{
	char* respuesta = malloc(sizeof(char));

	pthread_rwlock_wrlock(&lockTablaArchivos);
	respuesta[0] = renombrarArchivo(paths);
	pthread_rwlock_unlock(&lockTablaArchivos);

	free(paths);
	paths = NULL;
	if(respuesta[0] == 's')
	{
		return respuesta;
	}
	else
	{
		return NULL;
	}
}

void* procesarPedidoRmdir(char *path)
{
	char* respuesta = malloc(sizeof(char));

	pthread_rwlock_wrlock(&lockTablaArchivos);
	respuesta[0] = borrarDirectorio(path);
	pthread_rwlock_unlock(&lockTablaArchivos);

	free(path);
	path = NULL;
	return respuesta;
}

void* procesarPedidoTruncate(off_t newSize, char* path)
{
	//printf(CYN "\t En procesarPedidoTruncate el nuevo size es: %d\n", (uint32_t)newSize);
	char* respuesta = malloc(sizeof(char));

	pthread_rwlock_wrlock(&lockTablaArchivos);
	respuesta[0] = buscarYtruncar(path, (uint32_t)newSize);
	pthread_rwlock_unlock(&lockTablaArchivos);

	return respuesta;
}

void* procesarPedidoUnlink(char* path)
{
	char* respuesta = malloc(sizeof(char));

	pthread_rwlock_wrlock(&lockTablaArchivos);
	respuesta[0] = borrarArchivo(path);
	pthread_rwlock_unlock(&lockTablaArchivos);

	free(path);
	path = NULL;
	return respuesta;
}

void* procesarPedidoUtimens(char *path)
{
	char* respuesta = malloc(sizeof(char));
	printf(YEL "\t path: %s\n" RESET, path);

	pthread_rwlock_wrlock(&lockTablaArchivos);
	respuesta[0] = cambiarUltimoAcceso(path);
	pthread_rwlock_unlock(&lockTablaArchivos);

	free(path);
	path = NULL;
	return respuesta;
}

void* procesarPedidoWrite(void *buffer, int* codigo)
{
	int desplazamiento = 0;
	size_t* size = malloc(sizeof(size_t));
	off_t* offset = malloc(sizeof(off_t));
	int pathLen;
	int* bufLen = malloc(sizeof(int));

	memcpy(size, buffer, sizeof(size_t));
	desplazamiento += sizeof(size_t);
	memcpy(offset, buffer + desplazamiento, sizeof(off_t));
	desplazamiento += sizeof(off_t);
	memcpy(&pathLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(bufLen, buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);

	char* path = malloc(pathLen);
	memcpy(path,  buffer + desplazamiento, pathLen);
	desplazamiento += pathLen;
	void* bufWrite = malloc(*bufLen);
	memcpy(bufWrite,  buffer + desplazamiento, *bufLen);

	printf( "\t size: %d bytes\n", *size);
	printf( "\t offset: %d bytes\n", (uint32_t)*offset);
	printf(FUC "\t path: %s\n" RESET, path);

	free(buffer);
	buffer = NULL;
	int respuesta  = 0;
	if (*size > 0)
	{
		if(*bufLen > *size)
		{
			pthread_rwlock_wrlock(&lockTablaArchivos);
			respuesta = writeBuffer((uint32_t*)size,(uint32_t*) offset, path, bufWrite);
			pthread_rwlock_unlock(&lockTablaArchivos);
		}
		else
		{
			*bufLen -= 1;
			pthread_rwlock_wrlock(&lockTablaArchivos);
			respuesta =  writeBuffer((uint32_t*)bufLen, (uint32_t*)offset, path, bufWrite);
			pthread_rwlock_unlock(&lockTablaArchivos);
		}
	}

	free(offset);
	free(path);
	free(bufWrite);
	free(bufLen);
	offset=NULL;
	path=NULL;
	bufWrite=NULL;
	bufLen=NULL;
	switch(respuesta)

	{
		case 0:
			*codigo = RESPUESTA_WRITE;
			break;

		case -1://no existe path
			*codigo = ENOENTRY;
	    	break;

		case  -2:// no hay bloques libres
			*codigo = ERRNOSPC;
			break;

		case -3: //el tamaño a escribir excede los limites del file system
			*codigo = ERRFBIG;
			break;
	}

	return size;
}

//void threadsDestroyer(pthread_t* thread)
//{
//	free(thread);
//	thread = NULL;
//}

void terminar()
{
	printf(RED "\n\n****************** Señal SIGINT *************************************************\n" RESET);
	printf(RED     "**********************************************************************************\n" RESET);

	destruirSemaforos();
	sem_destroy(&semThreads);

	queue_destroy_and_destroy_elements(threadQueue, free);//(void*)threadsDestroyer);

	pthread_join(thread1, NULL);
	pthread_detach(thread1);

	close(listenningSocket);
	liberarRecursos();
	descargar();

	printTerminar();
	exit(0);
}


