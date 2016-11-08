/*
 * main_cliente.c
 *
 *  Created on: 5/9/2016
 *      Author: utnso
 */

#include "cliente-pokedex.h"


int main(int argc, char *argv[]) {


	if (chequear_argumentos(argc, TOTAL_ARGS) == -1)
		return EXIT_FAILURE;

	directorio_montaje = string_duplicate(argv[1]);
	printf("Cliente Pokedex en el directorio de montaje: %s\n", directorio_montaje);

	if (set_datos_conexion() == -1){
		puts("No se pudo establecer la conexion con el servidor");
		return -1;
	}
	//creo log
	logCliente = log_create("logPokedex", "Pokedex", false, LOG_LEVEL_DEBUG);
	log_info(logCliente, "****************** Creando archivo Log *******************************************" );

	printEncabezado();
	socketServidor = conectar_con_servidor_pkdx();
	obtenerFecha();


	//Inicializar Fuse
	PrintFuse();
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	return fuse_main(args.argc,args.argv, &osada_oper, NULL);

}
