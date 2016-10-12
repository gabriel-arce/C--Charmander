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

void validar(){

}

void crearLog(){
	logger = log_create(NOMBRE_LOG, NOMBRE_PROG, 0, LOG_LEVEL_INFO);
	log_info(logger, "Iniciando cliente pokedex...");
}

void crearSemaforos(){

}

void crearServer(){
	int fdmax; // número máximo de descriptores de fichero
	int listener; // descriptor de socket a la escucha
	int newfd; // descriptor de socket de nueva conexión aceptada
	int nbytes;
	int yes=1; // para setsockopt() SO_REUSEADDR, más abajo
	int i;

	fd_set readset;
	sigset_t mask, orig_mask;

	FD_ZERO(&master); // borra los conjuntos maestro y temporal
	FD_ZERO(&read_fds);

	// obtener socket a la escucha
	if ((listener = crearSocket()) == -1)
		exit(1);

	// estado del Fd para bind
	if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1)	{
		perror("setsockopt");
		exit(1);
	}

	// enlazar
	if (bindearSocket(listener,PUERTO, IP) == -1)
		exit(1);

	// escuchar
	if (escucharEn(listener) == -1)
		exit(1);

	//Nos aseguramos de que pselect no sea interrumpido por señales
	sigemptyset(&mask);
	sigaddset (&mask, SIGUSR1);
	sigaddset (&mask, SIGUSR2);
	sigaddset (&mask, SIGPOLL);

	if (sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
		perror ("sigprocmask Fallo");

	}

	// añadir listener al conjunto maestro
	FD_SET(listener, &master);

	fdmax = listener; // por ahora es éste
	// bucle principal
	while(1)	{

		read_fds = master; // cópialo
		if (pselect (fdmax+1,&read_fds, NULL, NULL, NULL, NULL) == -1){
			perror("select");
			exit(1);
		}

		// explorar conexiones existentes en busca de datos que leer
		for(i = 0; i <= fdmax; i++){
			if (FD_ISSET(i, &read_fds))
			{ // ¡¡tenemos datos!!
				if (i == listener){
					// gestionar nuevas conexiones
					if ((newfd = aceptarEntrantes(listener)) == -1)
						perror("accept");

					else {
						FD_SET(newfd, &master); // añadir al conjunto maestro

						if (newfd > fdmax) // actualizar el máximo
							fdmax = newfd;
					}
				} else {
					// gestionar datos de un cliente
					if ((nbytes = recv(i, buffer, sizeof(buffer), 0)) <= 0){
						// error o conexión cerrada por el cliente

						if (nbytes == 0)
							// conexión cerrada
							printf("selectserver: socket %d desconectado\n", i);

						else
							perror("recv");

						close(i); // ¡Hasta luego!

						FD_CLR(i, &master); // eliminar del conjunto maestro
					}

					else {
					// tenemos datos de algún cliente
						if (FD_ISSET(i, &master))	{
							//TODO Resolver esta parte..
						}
					}
				}

			}
		}
	}
}
