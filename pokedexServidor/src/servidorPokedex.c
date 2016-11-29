/*
* servidorPokedex.c
*
*  Created on: 29/9/2016
*      Author: Guadalupe
*/

#include "servidorPokedex.h"

t_list *ListaArchivos;

int main(int argc, char ** argv)
{
	ListaArchivos = list_create();

	printEncabezado();
	inicializarDisco();

	ListaArchivos = leerTablaArchivos();
	mostrar_lista_archivos();

	pthread_mutex_lock(&mutex_comunicacion);
		listenningSocket = crearServer(PUERTO);
	pthread_mutex_unlock(&mutex_comunicacion);

	printf( "\n****************** Creando hilos servidores ********************************\n\n");

	pthread_t thread1, thread2, thread3, thread4, thread5;

	pthread_create(&thread1, NULL, hiloComunicacion, NULL);
	pthread_create(&thread2, NULL, hiloComunicacion, NULL);
	pthread_create(&thread3, NULL, hiloComunicacion, NULL);
	pthread_create(&thread4, NULL, hiloComunicacion, NULL);
	pthread_create(&thread5, NULL, hiloComunicacion, NULL);

	signal(SIGINT,terminar);
	signal(SIGTERM,terminar);

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);
	pthread_join(thread3, NULL);
	pthread_join(thread4, NULL);
	pthread_join(thread5, NULL);

	pthread_detach(thread1);
	pthread_detach(thread2);
	pthread_detach(thread3);
	pthread_detach(thread4);
	pthread_detach(thread5);

	close(listenningSocket);
	descargar();

	printTerminar();
	return(0);
}

void* hiloComunicacion(void* arg)
{
	int head;
	void *mensaje = NULL;

	printf( "****************** Un hilo empieza a escuchar conexiones *******************\n\n" );

	while(1)
	{
		pthread_mutex_lock(&mutex_comunicacion);
			int socketCliente = aceptarConexion(listenningSocket);
		pthread_mutex_unlock(&mutex_comunicacion);

		mensaje = recibir(socketCliente,&head);
		char* mensajeHSK = mensaje;
		printf("\t Recibiendo pedido de un cliente:%s \n ", mensajeHSK);
		if (mensajeHSK)
		{
			if (enviar(socketCliente, HANDSHAKE, mensajeHSK) == -1)
			{
				printf(YEL "\t El cliente %d se desconecto antes de recibir el handshake \n " RESET, socketCliente);
			}
			else
			{
				printf("\t Devolviendo mensajeHSK %d \n", socketCliente);
				atendercliente(socketCliente);
			}
		}
		else
		{
			printf(YEL "\t Recibi una propuesta indecente de: %d :%s \n" RESET, socketCliente, mensajeHSK);
		}
	}
}

void atendercliente(int socket)
{
	int continuar = 1;
	int codigo = 0;

	while(continuar)
	{
		printf("******** Atiendo al cliente %d *********************************************\n", socket);

		void *respuesta = NULL;
		void *pedido = NULL;
		int head = 0;

		pedido = recibir(socket, &head);
		printf("\n******** Recibi del cliente " CYN "%d" RESET " un mensaje...\n", socket);

		if(pedido != NULL)
		{
			switch(head)
			{
				case PEDIDO_CREATE:
					printf(MAG "\t procesando PEDIDO_CREATE\n" RESET);

					codigo = RESPUESTA_CREATE;
					respuesta = procesarCrearEntradaTablaDeArchivos((char*)pedido, &codigo, 1);

					switch(codigo)
					{
						case RESPUESTA_CREATE:
							enviar(socket, RESPUESTA_CREATE, respuesta);
							printf(MAG "\t devolviendo RESPUESTA_CREATE\n" RESET);
							break;

						case ERRDQUOT:
							enviar(socket, ERRDQUOT, respuesta);
							printf(YEL "\t devolviendo ERRDQUOT\n" RESET);
							break;

						case ERRNAMETOOLONG:
							enviar(socket, ERRNAMETOOLONG, respuesta);
							printf(YEL "\t devolviendo ERRNAMETOOLONG\n" RESET);
							break;

						case RESPUESTA_ERROR:
							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(MAG "\t respondiendo  \n" RESET);
							break;

					    default:
							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(PINK2 "\t respondiendo  \n" RESET);
							break;
					}

					free(pedido);
					break;

				case PEDIDO_GETATTR:
					printf( "\t procesando PEDIDO_GETATTR\n" );

					respuesta = procesarPedidoGetatrr((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_GETATTR, respuesta);
						free(pedido);
						printf( "\t devolviendo RESPUESTA_GETATTR \n" );
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf("\t devolviendo respuesta ENOENT \n");
					}
					break;

				case PEDIDO_FLUSH:
					printf(GRN "\t procesando PEDIDO_FLUSH\n" RESET);

					respuesta = procesarPedidoFlush((char*)pedido);
					if(respuesta != NULL)
					{
						free(pedido);
						enviar(socket, RESPUESTA_FLUSH, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_FLUSH \n" RESET);
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_MKDIR:
					printf(MAG "\t procesando PEDIDO_MKDIR\n" RESET);

				    codigo = RESPUESTA_MKDIR;
					respuesta = procesarCrearEntradaTablaDeArchivos((char*)pedido, &codigo, 2);

					switch(codigo)
					{
						case RESPUESTA_CREATE:
							enviar(socket, RESPUESTA_MKDIR, respuesta);
							printf(MAG "\t devolviendo RESPUESTA_MKDIR\n" RESET);
							break;

						case ERRDQUOT:
							enviar(socket, ERRDQUOT, respuesta);
							printf(YEL "\t devolviendo ERRDQUOT\n" RESET);
							break;

						case ERRNAMETOOLONG:
							enviar(socket, ERRNAMETOOLONG, respuesta);
							printf(YEL "\t devolviendo ERRNAMETOOLONG\n" RESET);
							break;

						case RESPUESTA_ERROR:
							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(MAG "\t respondiendo  \n" RESET);
							break;

					    default:
							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(MAG "\t respondiendo  \n" RESET);
							break;
					}
					free(pedido);
					break;

				case PEDIDO_MKNOD:
					printf(MAG "\t procesando PEDIDO_MKNOD\n" RESET);

					codigo = RESPUESTA_MKNOD;
					respuesta = procesarCrearEntradaTablaDeArchivos((char*)pedido, &codigo, 1);

					switch(codigo)
					{
						case RESPUESTA_CREATE:
							enviar(socket, RESPUESTA_MKNOD, respuesta);
							printf(MAG "\t devolviendo RESPUESTA_MKNOD\n" RESET);
							break;

						case ERRDQUOT:
							enviar(socket, ERRDQUOT, respuesta);
							printf(YEL "\t devolviendo ERRDQUOT\n" RESET);
							break;

						case ERRNAMETOOLONG:
							enviar(socket, ERRNAMETOOLONG, respuesta);
							printf(YEL "\t devolviendo ERRNAMETOOLONG\n" RESET);
							break;

						case RESPUESTA_ERROR:
							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(MAG "\t respondiendo  \n" RESET);
							break;
					    default:
							enviar(socket, RESPUESTA_ERROR, respuesta);
							printf(PINK2 "\t respondiendo  \n" RESET);
							break;
					}
					free(pedido);
					break;

				case PEDIDO_OPEN:
					printf(GRN "\t procesando PEDIDO_OPEN\n" RESET);
					codigo = RESPUESTA_OPEN;

					respuesta = procesarPedidoOpen((char*)pedido, &codigo);
					if(codigo == RESPUESTA_OPEN)
					{
						enviar(socket, RESPUESTA_OPEN, respuesta);
						printf(GRN "\t devolviendo RESPUESTA_OPEN\n" RESET);
					}
					else if(codigo == ENOENTRY)
					{
						enviar(socket, ENOENTRY, respuesta);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
//					else//devolver algun codigo que indique que el archivo esta bloqueado
//					{
//						enviar(socket, BLOQUEADO, respuesta);
//						printf(RED "\t devolviendo respuesta BLOQUEADO \n" RESET);
//					}
					break;

				case PEDIDO_READ:
					printf( BLU "\t procesando PEDIDO_READ\n" RESET);

					void *buffer = NULL;
					uint32_t* tamanioBuffer = malloc(sizeof(uint32_t));
					memset(tamanioBuffer, 0, sizeof(uint32_t));

					buffer = recibirEstructuraRead(socket, &head);
					respuesta = procesarPedidoRead(buffer, tamanioBuffer);

					if(respuesta != NULL)
					{
						free(pedido);
						enviarRespuestaRead(socket, RESPUESTA_READ, respuesta, tamanioBuffer);
						printf(BLU "\t devolviendo RESPUESTA_READ \n" RESET);
					}
					else
					{
						free(tamanioBuffer);
						enviar(socket, ENOENTRY, pedido);
						printf(YEL "\t devolviendo respuesta ENOENT \n" RESET);
					}
					break;

				case PEDIDO_READDIR:
					printf( "\t procesando PEDIDO_READDIR\n" );

					respuesta = procesarPedidoReaddir((char*)pedido);
					if(respuesta != NULL)
					{
						free(pedido);
						enviar(socket, RESPUESTA_READDIR, respuesta);
						printf( "\t devolviendo RESPUESTA_READDIR\n" );
					}
					else
					{
						enviar(socket, ENOENTRY, pedido);
						printf("\t devolviendo respuesta ENOENT \n");
					}
					break;

				case PEDIDO_RELEASE:
					printf(GRN "\t procesando PEDIDO_RELEASE\n" RESET);

					respuesta = procesarPedidoRelease((char*)pedido);
//					if(respuesta != NULL)
//					{
					enviar(socket, RESPUESTA_RELEASE, respuesta);
					printf(GRN "\t devolviendo RESPUESTA_RELEASE\n" RESET);
//					}
//					else
//					{
//						enviar(socket, RESPUESTA_ERROR, pedido);
//						printf(YEL "\t devolviendo RESPUESTA_ERROR \n" RESET);
//					}
					break;

				case PEDIDO_RENAME:
					printf(NAR "\t procesando PEDIDO_RENAME\n" RESET);
					respuesta = procesarPedidoRename((char*)pedido);
					if(respuesta != NULL)
					{
						enviar(socket, RESPUESTA_RENAME, respuesta);
						printf(NAR "\t devolviendo RESPUESTA_RENAME\n" RESET);
					}
					else
					{
						enviar(socket, ERRNAMETOOLONG, pedido);
						printf(YEL "\t devolviendo respuesta ERRNAMETOOLONG \n" RESET);
					}
					break;

				case PEDIDO_RMDIR:
					printf(PINK "\t procesando PEDIDO_RMDIR\n" RESET);

					respuesta = procesarPedidoRmdir((char*)pedido);
//					if (respuesta != NULL)
//					{
					//free(pedido);
					enviar(socket, RESPUESTA_RMDIR, respuesta);
					printf(PINK "\t devolviendo RESPUESTA_RMDIR\n" RESET);
//					}
//					else
//					{
//						enviar(socket, RESPUESTA_ERROR, pedido);
//						printf(YEL "\t devolviendo RESPUESTA_ERROR \n" RESET);
//					}
					break;

				case PEDIDO_TRUNCATE:
					printf(NAR "\t procesando PEDIDO_TRUNCATE\n" RESET);

					off_t *newSize = NULL;
					newSize = (off_t*)recibir(socket, &head);

					if (head == PEDIDO_TRUNCATE_NEW_SIZE)
					{
						respuesta = procesarPedidoTruncate(*newSize, (char*)pedido);
					}
					if(respuesta != NULL)
					{
						free(pedido);
						enviar(socket, RESPUESTA_TRUNCATE, respuesta);
						printf(NAR "\t devolviendo RESPUESTA_TRUNCATE\n" RESET);
					}
					else
					{
						enviar(socket, RESPUESTA_ERROR, pedido);
						printf(YEL "\t devolviendo RESPUESTA_ERROR \n" RESET);
					}
					free(newSize);
					break;

				case PEDIDO_UNLINK:
					printf(PINK "\t procesando PEDIDO_UNLINK\n" RESET);

					respuesta = procesarPedidoUnlink((char*)pedido);
//					if(respuesta != NULL)
//					{
					enviar(socket, RESPUESTA_UNLINK, respuesta);
					printf(PINK "\t devolviendo RESPUESTA_UNLINK\n" RESET);
//					}
//					else
//					{
//						enviar(socket, RESPUESTA_ERROR, pedido);
//						printf(YEL "\t devolviendo RESPUESTA_ERROR \n" RESET);
//					}
					break;

				case PEDIDO_UTIMENS:
					printf(NAR "\t procesando UTIMENS \n" RESET);

					respuesta = procesarPedidoUtimens((char*)pedido);
//					if(respuesta != NULL)
//					{
					enviar(socket, RESPUESTA_UTIMENS, respuesta);
					printf(NAR "\t devolviendo RESPUESTA_UTIMENS \n" RESET);
//					}
//					else
//					{
//						enviar(socket, RESPUESTA_ERROR, pedido);
//						printf(YEL "\t devolviendo RESPUESTA_ERROR \n" RESET);
//					}
					break;

				case PEDIDO_WRITE:
					printf( PINK "\t procesando PEDIDO_WRITE\n" RESET);
					codigo = RESPUESTA_WRITE;
					void *bufWrite = NULL;

					bufWrite = recibirEstructuraWrite(socket, &head);
					respuesta = procesarPedidoWrite(bufWrite, &codigo);

					switch(codigo)
					{
						case RESPUESTA_WRITE:
							enviar(socket, RESPUESTA_WRITE, respuesta);
							printf(PINK "\t devolviendo RESPUESTA_WRITE \n" RESET);
							break;

						case ERRFBIG:
							enviar(socket, ERRFBIG, respuesta);
							printf(YEL "\t devolviendo ERRFBIG \n" RESET);
							break;

					    default:
							enviar(socket, ENOENTRY, respuesta);
							printf(PINK2 "\t respondiendo  \n" RESET);
							break;
					}
					free(pedido);
					break;

				default:
					printf(RED "\n¿Porqué entre en default???, ¿tenia que enviar un handshake por segunda vez??? \n\n" RESET);
					enviar(socket,HANDSHAKE, pedido);
					break;
			}

		}
		else
		{
			printf(YEL "\n******** Se desconecto el cliente %d, buscando uno nuevo para atender ******\n" RESET, socket);
			continuar = 0;
		}
	}//fin while
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
	char* respuesta = malloc(sizeof(char));
	char r = crearArchivo(path, modo);
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
	return getAttr(path);
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
	//respuesta[0] = abrirArchivo(path);

	printf("Consultando fichero %s.. \n", path);

	respuesta[0] = verificar_permiso_archivo(path);

	if(respuesta[0] == 's')
		*codigo = RESPUESTA_OPEN;
	if (respuesta[0] == 'n')
		*codigo = ENOENTRY;
//	else
//		*codigo = BLOQUEADO;

	free(path);
	return respuesta;
}

char verificar_permiso_archivo(char *path){
	int i;
	char respuesta;
	char *aux = malloc(OSADA_FILENAME_LENGTH);
	t_nodoArchivo *nodo;

	aux = nombre(path);

	for(i = 0; i < list_size(ListaArchivos); i++){
		nodo = list_get(ListaArchivos, i);
		if(strcmp(nodo->nombre,aux) == 0){
			//Lo encontro..
			if (nodo->enUso == SinUso)
				respuesta = 's';	//Se puede utilizar
			else
				respuesta = 'n';

			break;
		}
	}

	if (strcmp(nodo->nombre,aux) != 0)	//No lo encontro, entonces no existe el fichero..
		respuesta = 'n';

	return respuesta;
}

void mostrar_lista_archivos(){
	int i;
	t_nodoArchivo *nodo;

	nodo			= malloc(sizeof(t_nodoArchivo));
	nodo->nombre	= malloc(OSADA_FILENAME_LENGTH);

	printf("Ficheros actualmente en el FS..\n");
	for(i = 0; i < list_size(ListaArchivos); i++){
		nodo = list_get(ListaArchivos, i);
		printf("Fichero: %s\n", nodo->nombre);
	}

	if (i == 0)
		printf("No hay archivos en la lista..\n");

	//free(nodo->nombre);
	//free(nodo);
}

void* procesarPedidoRelease(char* path)
{
	printf("\t path: %s\n", path);
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = liberarArchivo(path);
	free(path);
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

	void* respuesta = readBuffer(path, size, offset, tamanioBuffer);

	free(buffer);
	free(path);
	free(size);
	free(offset);

	return respuesta;

}

void* procesarPedidoReaddir(char *path)
{
	printf("\t path: %s\n", path);
	return readdir(path);
}

void* procesarPedidoRename(char *paths)//path - newpath
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = renombrarArchivo(paths);
	free(paths);
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
	respuesta[0] = borrarDirectorio(path);
	free(path);
	return respuesta;
}

void* procesarPedidoTruncate(off_t newSize, char* path)
{
	printf(CYN "\t En procesarPedidoTruncate el nuevo size es: %d\n", (uint32_t)newSize);
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = buscarYtruncar(path, (uint32_t)newSize);
	return respuesta;
}

void* procesarPedidoUnlink(char* path)
{
	char* respuesta = malloc(sizeof(char));
	respuesta[0] = borrarArchivo(path);
	free(path);
	return respuesta;
}

void* procesarPedidoUtimens(char *path)
{
	char* respuesta = malloc(sizeof(char));
	printf(YEL "\t path: %s\n" RESET, path);
	respuesta[0] = cambiarUltimoAcceso(path);
	free(path);
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

	//printf(BLU "\t En procesarPedidoWrite el size es: %d\n", *size);
	//printf( "\t En procesarPedidoWrite el offset es: %d\n", (int)*offset);
	//printf( "\t En procesarPedidoWrite el pathlen es: %d\n", pathLen);
	//printf( "\t En procesarPedidoWrite el bufLen es: %d\n", *bufLen);
	//printf( "\t En procesarPedidoWrite el path es: %s\n" RESET, path);

	free(buffer);

	int respuesta  = 0;
	if(*bufLen > *size)
	{
		respuesta = writeBuffer((uint32_t*)size,(uint32_t*) offset, path, bufWrite);
	}
	else
	{
		*bufLen -= 1;
		respuesta =  writeBuffer((uint32_t*)bufLen, (uint32_t*)offset, path, bufWrite);
	}

	free(offset);
	free(path);
	free(bufWrite);
	free(bufLen);

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

void terminar()
{
	close(listenningSocket);

	descargar();
	liberarRecursos();
	printf(RED "\n\n****************** Señal SIGTERM *************************************************\n" RESET);
	printf(RED     "**********************************************************************************\n" RESET);
	printTerminar();

	exit(0);
}


