/*
 ============================================================================
 Name        : servidor-pokedex.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "servidor-pokedex.h"


//------------Server
void validar(){

}

void inicializarVariables() {
	server_on = true;
	clientes_conectados = list_create();
}

void crearLog(){
	logger = log_create(NOMBRE_LOG, NOMBRE_PROG, 0, LOG_LEVEL_INFO);
	log_info(logger, "Iniciando cliente pokedex...");
}

void crearSemaforos(){
	pthread_mutex_init(&mutex_servidor, 0);
}

void destruirSemaforos() {
	pthread_mutex_destroy(&mutex_servidor);
}

void crearServer() {

	int optval = 1;

	fd_servidor = crearSocket();

	if (fd_servidor == -1)
		exit(EXIT_FAILURE);

	if ( bindearSocket(fd_servidor, PUERTO, IP) == -1 )
		exit(EXIT_FAILURE);

	if ( escucharEn(fd_servidor) == -1 )
		exit(EXIT_FAILURE);

	struct sockaddr_in direccion_cliente;
	unsigned int addrlen = sizeof(struct sockaddr_in);
	int new_fd;

	while(server_on) {
		pthread_mutex_lock(&mutex_servidor);

		new_fd = accept(fd_servidor, (struct sockaddr *) &direccion_cliente, &addrlen);

		if (new_fd == -1) {
			pthread_mutex_unlock(&mutex_servidor);
			continue;
		}

		setsockopt(new_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

		printf("\nNueva conexion de un cliente en el socket: %d\n", new_fd);

		t_sesion_cliente * nuevo_cliente = crearSesionCliente(new_fd);

		pthread_t subhilo_cliente;

		pthread_create(&subhilo_cliente, NULL, (void *) serverCliente, (void *) nuevo_cliente);

		//TODO en que momento le envia el punto de montaje????

		pthread_mutex_unlock(&mutex_servidor);
	}

}

t_sesion_cliente * crearSesionCliente(int cli_socket) {
	t_sesion_cliente * nueva_sesion_cliente = malloc(sizeof(t_sesion_cliente));

	nueva_sesion_cliente->socket_cliente = cli_socket;

	return nueva_sesion_cliente;
}

void * serverCliente(void * args) {
	t_sesion_cliente * cliente = (t_sesion_cliente *) args;

	bool client_is_connected = true;
	pthread_mutex_t mutex_cliente = PTHREAD_MUTEX_INITIALIZER;

	//TODO definir interfaces y protocolos de comunicacion con los clientes pkdx
	while (client_is_connected) {

		pthread_mutex_lock(&mutex_cliente);
		//recv + switch
		pthread_mutex_unlock(&mutex_cliente);

	}


//------------------File System




	return EXIT_SUCCESS;
}
