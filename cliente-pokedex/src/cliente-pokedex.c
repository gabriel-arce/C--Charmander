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

int main(int argc, char **argv){

	//Crear Log
	crear_logger();

	//Validar
	validar(argc, argv);

	//Mapear disco
	mapearDisco();

	//Cerrar disco
	cerrarDisco();

	//Conectar con el servidor
	conectar_con_servidor_pkdx();

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	operaciones.getattr		= tomar_atributos;
	operaciones.readdir		= leer_directorio;
	operaciones.open			= abrir;
	operaciones.read				= leer;
	operaciones.destroy		= limpiar;
	operaciones.mknod		= crear_nodo;
	operaciones.unlink			= borrar_archivo;
	operaciones.rename	 	= renombrar;
	operaciones.truncate	= cambiar_tamano;
	operaciones.write    		= escribir;
	//TODO Falta definir las siguientes funciones..
	operaciones.rmdir			= borrar_directorio;
	operaciones.mkdir			= crear_directorio;
	operaciones.create			= crear_archivo;
	operaciones.getxattr	= tomar_atributos_extendidos;
	operaciones.access		= verificar_acceso;

	return fuse_main ( args.argc, args.argv, &operaciones, NULL);


}



void crear_logger(){
	logger = log_create(NOMBRE_LOG, NOMBRE_PROG, 0, LOG_LEVEL_INFO);
	log_info(logger, "Iniciando cliente pokedex...");
}

int validar(int argc, char **argv){

	if ((in = open(argv[1], O_RDWR, 0777)) == -1){
		mostrarMensajeDeError(NoSePudoAbrirIn);
		return EXIT_FAILURE;
	}

	// Verifica sin argumentos
	if (argc == 1){
		mostrarAyuda();
		return EXIT_SUCCESS;
		}

	// Verifica cantidad de argumentos
	if(argc != 2){
		mostrarMensajeDeError(CantidadArgumentosIncorrecta);
		return EXIT_FAILURE;
	}

	// Stat del archivo
	if ((stat(argv[1], &fileStat)) < 0){
		mostrarMensajeDeError(ErrorStat);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

void mostrarAyuda(){
	puts("Ayuda");
}

void mostrarMensajeDeError(Error e){
	switch(e){
		case CantidadArgumentosIncorrecta:
			log_debug(logger, "Cantidad de argumentos incorrecta..");
			break;

		case NoSePudoAbrirIn:
			puts("No se pudo abrir archivo de entrada..");
			log_debug(logger, "No se pudo abrir archivo de entrada..");
			break;

		case ErrorEnLectura:
			puts("Error en lectura..");
			log_debug(logger, "Error en lectura..");
			break;

		case ErrorEnEscritura:
			puts("Error en escritura..");
			log_debug(logger, "Error en escritura..");
			break;

		case OtroError:
			puts("Error desconocido..");
			log_debug(logger, "Error desconocido..");
			break;

		case ErrorStat:
			puts("Error al leer stat del archivo..");
			log_debug(logger, "Error al leer stat del archivo..");
			break;

		case ErrorMmap:
			perror("mmap");
			printf("Error al mapear estructura..\n\n");
			log_debug(logger, "Error al mapear estructura..\n\n");
			break;

	}
}

static int tomar_atributos ( const char *path, struct stat *stbuf ){
	t_listaArchivos *actual = ListaArchivos;

	//Raiz
	if(strcmp(path, "/") == 0){
		//Relleno la estructura con 0s
		memset(stbuf, 0, sizeof( struct stat));
		stbuf->st_mode	=	S_IFDIR	|	0777;
		stbuf->st_nlink	=	2;
		return 0;
	}

	while(actual){
		//Otro directorio
		if(strcmp(path, actual->nombre) == 0){
			memcpy(stbuf, &actual->info, sizeof(struct stat));
			return 0;
		}

		//Siguiente
		actual = actual->siguiente;
	}

	//Si llego aca, no lo encontro..
	return -ENOENT;

}

static int leer_directorio ( const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi ){
	t_listaArchivos *actual = ListaArchivos;

	//Raiz
	if(strcmp(path, "/") == 0){
		return -ENOENT;
	}

	filler(buf, ".", NULL, 0);		//Entrada para .
	filler(buf, "..", NULL, 0);		//Entrada para ..

	while(actual){
		//Entrada de archivos
		if((filler(buf, actual->nombre + 1, &actual->info, 0 )) == 1)
			return 1;

		//Siguiente
		actual = actual->siguiente;
	}

	return 0;
}

static int abrir ( const char *path, struct fuse_file_info *fi ){
	t_listaArchivos *actual = ListaArchivos;

	//Recorrer lista
	while(actual){
		if((strcmp(path, actual->nombre)) == 0)
			return 0;

		//Siguiente
		actual = actual->siguiente;
	}

	//Si llego hasta aca, error
	return -ENOENT;

}

static int leer ( const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi ){
	t_listaArchivos *actual = ListaArchivos;
	int fd, leido;

	//Recorrer lista
	while(actual){
		if((strcmp(path, actual->nombre)) == 0)
			break;

		//Siguiente
		actual = actual->siguiente;
	}

	//Fin de lista y no encontro el archivo
	if(!actual)
		return -ENOENT;

	//Abrir archivo
	if ((fd = open( actual->path, O_RDONLY)) == -1)
		return -ENOENT;

	//Busco
	lseek(fd, offset, SEEK_SET);

	//Leo el archivo
	leido = read(fd, buf, size);

	//Cierro fd
	close(fd);

	return leido;

}

static void limpiar ( void *datos ){
	t_listaArchivos *actual = ListaArchivos;
	t_listaArchivos *anterior;

	while(actual){
		free( actual->nombre);
		free( actual->path);

		//Ir al siguiente
		anterior = actual;
		actual = anterior->siguiente;
		free(anterior);
	}

}

static int borrar_archivo ( const char *path ){
	t_listaArchivos *actual			= ListaArchivos;
	t_listaArchivos *anterior	= ListaArchivos;

	while(actual){
		if((strcmp(actual->nombre, path)) == 0)
			break;

		anterior = actual;
		//Siguiente
		actual = actual->siguiente;
	}

	if(!actual)
		return -ENOENT;

	if(anterior == actual)
		ListaArchivos = actual->siguiente;		//apunta al siguiente elemento
	else
		anterior->siguiente = actual->siguiente;

	//Libero
	free( actual->nombre );
	free( actual->path );
	free( actual );

	return 0;
}



static int renombrar ( const char *viejo, const char *nuevo ){
	t_listaArchivos *actual = ListaArchivos;

	while(actual){
		if((strcmp(actual->nombre, viejo)) == 0)
			break;

		//Siguiente
		actual = actual->siguiente;
	}

	//No se encontro..
	if(!actual)
		return -ENOENT;

	//Elimino el nombre viejo
	free( actual->nombre );

	//Reservo para el nombre nuevo
	if ((actual->nombre = malloc(strlen(nuevo) +1)) == NULL)
		return EXIT_FAILURE;

	//Copio nombre nuevo
	strcpy(actual->nombre, nuevo);

	return 0;
}



static int crear_nodo ( const char *path, mode_t modo, dev_t dispositivo ){
	t_listaArchivos *actual			= ListaArchivos;
	t_listaArchivos *anterior	= ListaArchivos;

	while(actual){
		if((strcmp(actual->nombre, path)) == 0)
			return -EEXIST;

		anterior = actual;
		actual = actual->siguiente;
	}

	if(!ListaArchivos){		//Lista vacia
		if ((ListaArchivos = malloc(sizeof (t_listaArchivos))) == NULL)
			return EXIT_FAILURE;
		actual = ListaArchivos;
	}

	else{
		if ((anterior->siguiente = malloc(sizeof(t_listaArchivos))) == NULL)
			return EXIT_FAILURE;
		actual = anterior->siguiente;

	}

	//Reservo para el nombre
	if ((actual->nombre = malloc(strlen(path) +1)) == NULL)
		return EXIT_FAILURE;

	//Copio nombre
	strcpy(actual->nombre, path);

	//Permisos
	memset(&actual->info, 0, sizeof(struct stat));

	actual->info.st_mode	= S_IFREG	|	0666;
	actual->info.st_nlink		= 1;

	//Configurar path y siguiente
	actual->path			= NULL;
	actual->siguiente	=	NULL;

	return 0;

}

static int cambiar_tamano ( const char *path, off_t tamanio ){
	t_listaArchivos *actual = ListaArchivos;
	int ret;

	while(actual){
		if((strcmp(actual->nombre, path)) == 0)
			break;

		actual = actual->siguiente;
	}

	if(!actual)
		return -ENOENT;

	//Si no apunta a ningun lado?
	if(actual->path == NULL)
		return 0;

	//Trunco
	if ((ret = truncate( actual->path, tamanio)) == -1)
		return -errno;

	actual->info.st_size = tamanio;

	return ret;

}

static int escribir ( const char *path, const char *buffer, size_t tamanio, off_t offset, struct fuse_file_info *nada ){
	t_listaArchivos *actual = ListaArchivos;
	int fd, cnt;

	while(actual){
		if((strcmp(actual->nombre, path)) == 0)
			break;

		actual = actual->siguiente;
	}

	if(!actual)
		return -ENOENT;

	if(actual->path == NULL){
		char *ptr = strchr(buffer, '/');
		struct stat datos;

		//Si no tiene un /, no es un path.. puede ser que no sea absoluto
		if(!ptr)
			return -EINVAL;

		//Sacar /n
		ptr = strchr(buffer, '/n');
		if(ptr){
			*ptr = '\0';
			tamanio--;
		}

		//Stat de archivo
		if ((stat(buffer, &datos)) == -1)
			return -errno;

		//Archivo regular?
		if( ! S_ISREG( datos.st_mode ))
			return -EINVAL;

		//Actualizar archivo
		if ((actual->path = malloc(tamanio)) == -1)
			return EXIT_FAILURE;

		strncpy( actual->path, buffer, tamanio);

		actual->info = datos;

		return 0;
	}

	//Abrir archivo para escribir
	if ((fd = open( actual->path, O_RDWR)) == -1)
		return -errno;

	//Posicionar
	if ((lseek(fd, offset, SEEK_SET)) == -1){
		close(fd);
		return -EINVAL;
	}

	//Escribir
	if ((cnt = write(fd, buffer, tamanio)) == -1){
		close(fd);
		return -errno;
	}

	//Cerrar archivo
	close(fd);

	//Actualizar tamanio de archivo
	if (offset + tamanio > actual->info.st_size)
		actual->info.st_size = offset + tamanio;

	return cnt;

}


static int borrar_directorio ( const char *path ){
	return 0;
}

static int crear_archivo(const char *path, mode_t modo, struct fuse_file_info *fi){
	return 0;
}

static int verificar_acceso(const char *path, int mask){
	struct fuse_context *context = fuse_get_context();
	void *privado = context->private_data;
	return privado != NULL;
}

static int crear_directorio( const char *path, mode_t modo){
	return 0;
}

static int tomar_atributos_extendidos(const char *path, const char *nombre, char *valor, size_t tamanio){
	return 0;
}

void mapearDisco(){

	hdr = mmap((caddr_t)0, fileStat.st_size, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, in, 0);

	if(hdr == MAP_FAILED){
		mostrarMensajeDeError(ErrorMmap);
	}
}

void cerrarDisco(){
	close(in);
}
