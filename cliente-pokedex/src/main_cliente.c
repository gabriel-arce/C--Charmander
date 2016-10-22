/*
 * main_cliente.c
 *
 *  Created on: 5/9/2016
 *      Author: utnso
 */

#include "cliente-pokedex.h"
/*

static int pkdx_getattr( const char * path, struct stat * st ) {

	int res;

	st->st_uid = getuid();
	st->st_gid = getgid();

	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
	} else if (strcmp(path, DEFAULT_FILE_PATH) == 0) {
		st->st_mode = S_IFREG | 0444;
		st->st_nlink = 1;
		st->st_size = 1024;
	} else {
		res = -ENOENT;
	}

	return res;
}

static int pkdx_readdir( const char * path, void * buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info * fi ) {

	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler( buffer, ".", NULL, 0 );
	filler( buffer, "..", NULL, 0 );
	//faltan los directorios/archivos default para mostrar

	return 0;
}

static int pkdx_read( const char * path, char * buffer, size_t size, off_t offset, struct fuse_file_info * fi) {
	size_t len;
	(void) fi;



	return size;
}

static int pkdx_open(const char *path, struct fuse_file_info *fi) {
	if (strcmp(path, DEFAULT_FILE_PATH) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static struct fuse_operations pkdx_fuse_operations = {
		.getattr = pkdx_getattr,
		.readdir = pkdx_readdir,
		.read = pkdx_read,
		.open = pkdx_open,
};

int main(int argc, char ** argv) {

	if (chequear_argumentos(argc, TOTAL_ARGS) == -1) {
		return EXIT_FAILURE;
	}

	directorio_montaje = string_new();
	string_append(&directorio_montaje, argv[1]);

	printf("\nCliente Pokedex en el directorio de montaje: %s\n", directorio_montaje);

	socket_pokedex = conectar_con_servidor_pkdx();


	return fuse_main(argc, argv, &pkdx_fuse_operations, NULL);
}
*/

int main(int argc, char ** argv) {

	if (chequear_argumentos(argc, TOTAL_ARGS) == -1)
		return EXIT_FAILURE;

	directorio_montaje = string_duplicate(argv[1]);

	if (set_datos_conexion() == -1)
		return -1;

	socket_pokedex = conectar_con_servidor_pkdx();

	//Inicializar Fuse
	return fuse_main(argc,argv, &operaciones_fuse, NULL);

	return EXIT_SUCCESS;
}
