/*
 ============================================================================
 Name        : cliente-pokedex.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "cliente-pokedex.h"

int conectar_con_servidor_pkdx() {
	int socket_fd = -1;

	socket_fd = clienteDelServidor(IP, PORT);

	return socket_fd;
}
