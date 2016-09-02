/*
 * sockets.h
 *
 *  Created on: 11/8/2016
 *      Author: utnso
 */

#ifndef SHARED_SOCKETS_H_
#define SHARED_SOCKETS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>     //memset
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>      //perror
#include <arpa/inet.h>  //INADDR_ANY
#include <unistd.h>     //close  usleep
#include <netdb.h> 		//gethostbyname
#include <netinet/in.h>
#include <fcntl.h> 		//fcntl
#include <commons/string.h>


#define COLADECONEXIONES 10			//Cantidad de conexiones

int crearSocket();											//Se crea el Fd del socket
int bindearSocket(int socketFd, int puerto, char * ip);		//Asocia el socketFd al puerto deseado
int escucharEn(int socketFd);								//Se pone empieza a escuchar en el socketFd asociado al puerto
int aceptarEntrantes(int socketFd);							//Se aceptan conexion, pero de a una
int conectarA(int socketFd, char* ipDestino, int puerto);	//se conecta al una direccion a traves del socketFd
int clienteDelServidor(char *ipDestino,int puerto);

#endif /* SHARED_SOCKETS_H_ */
