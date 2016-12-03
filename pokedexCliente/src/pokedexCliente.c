
/*
 * pokedexCliente.c
 *
 *  Created on: 17/9/2016
 *      Author: utnso
 */

#include "pokedexCliente.h"

int main(int argc, char *argv[])
{
  ip = getenv("SERVER_HOST");
  puerto = getenv("SERVER_PORT");

  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  int ret;
  int head = 0;

  printEncabezado();

  logCliente = log_create("logPokedex", "Pokedex", false, LOG_LEVEL_DEBUG);
  log_info(logCliente, "****************** Creando archivo Log *******************************************" );

  char* mensaje = malloc(sizeof(char)*4);
//  memset(ip,0,10);
//  memset(puerto,0,5);
  memset(mensaje,0,sizeof(char)*4);

//  strcpy(ip,"127.0.0.1");
//  strcpy(puerto,"4969");
  strcpy(mensaje,"hsk");

  signal(SIGINT,terminar);
  signal(SIGTERM,terminar);

  socketServidor = malloc(sizeof(int*));

  printf("****************** Estableciendo conexion con el servidor... \n\n");
  log_info(logCliente, "****************** Estableciendo conexion con el servidor... " );

  int socket = crearSocket(ip, puerto);

  if (socket == -1)
  {
    printErrorConexion();
    liberarRecursos();
    free(mensaje);
    return -1;
  }
  else if (socket == -2)
  {
    printServidorDesconectado();
    liberarRecursos();
    free(mensaje);
    return -1;
  }

  memset(socketServidor, 0, sizeof(int));
  *socketServidor = socket;
  printf("\t Enviando mensaje handshake al servidor \n");
  log_info(logCliente, "	Enviando mensaje handshake al servidor" );

  enviar(*socketServidor, HANDSHAKE, mensaje);
  void* respuesta =  recibir(*socketServidor,&head);
  free(respuesta);

  if (head == HANDSHAKE)
  {
    printConectado();
  }
  else
  {
    //no deberia entrar aca
    printMensajeInesperado(head);
  }

  PrintFuse();

  //Operaciones en FUSE
  osada_oper.access		= osada_access;		// accesos de un fichero
  osada_oper.create		= osada_create;		// crear y abrir un archivo
  osada_oper.flush		= osada_flush;
  osada_oper.getattr	= osada_getattr;	// obtener atributos
  osada_oper.mkdir		= osada_mkdir;		// crear un directorio
  osada_oper.mknod		= osada_mknod;		// crear un directorio
  osada_oper.open		= osada_open;		// abre un archivo
  osada_oper.read		= osada_read; 		// leer archivo
  osada_oper.readdir	= osada_readdir; 	// leer un directorio
  osada_oper.release	= osada_release;	// libera un archivo que estuvo abierto
  osada_oper.rename		= osada_rename;		// renombrar un archivo
  osada_oper.rmdir		= osada_rmdir;		// borrar un directorio
  osada_oper.truncate	= osada_truncate;	// redimensionar archivo
  osada_oper.unlink		= osada_unlink;		// borrar un archivo
  osada_oper.utimens	= osada_utimens;	// borrar un archivo
  osada_oper.write		= osada_write;		// escribir un archivo

  ret = fuse_main(args.argc, args.argv, &osada_oper, NULL);

  liberarRecursos();
  printTerminar();

  return ret;
}

void liberarRecursos()
{
	log_destroy(logCliente);
	free(socketServidor);
}

//funciones para gestionar los pedidos de FUSE---------------------------------------------------------------------------------
static int osada_access(const char *filename, int how)
{
  return 0;
}

static int osada_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	// crear y abrir un archivo
	log_info(logCliente, "FUSE: llamada a osada_create(), path: %s", path);

	int head = 0;
	void* paquete = NULL;

	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);
	log_info(logCliente, "	Envio PEDIDO_CREATE");

	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_CREATE, pathAux);
		paquete = recibir(*socketServidor, &head);
	pthread_mutex_unlock(&mutex_comunicacion);

	free(paquete);
	switch (head)
	{
		case RESPUESTA_CREATE:
			log_info(logCliente, "	Recibi RESPUESTA_CREATE");
			return 0;

		case ERRDQUOT:
			log_info(logCliente, "	Recibi respuesta EDQUOT en osada_create.......................................");
			return -EDQUOT;

		case ERRNAMETOOLONG:
			log_info(logCliente, "	Recibi respuesta ENAMETOOLONG en osada_create.................................");
			return -ENAMETOOLONG;

		case ENOENTRY:
			log_info(logCliente, "	No recibi RESPUESTA_CREATE en osada_create.....................................");
			return 0;
	}
	return 0;
}

static int osada_flush(const char *path, struct fuse_file_info *fi)
{
	log_info(logCliente, "FUSE: llamada a osada_flush(), path: %s", path);
	log_info(logCliente, "fuse_file_info *fi en osada_flush(), flags: %d", fi->flags);
	int head = 0;
	void* paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);

	log_info(logCliente, "	Envio PEDIDO_FLUSH");
	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_FLUSH, (void*)pathAux);
		paquete = recibir(*socketServidor, &head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_FLUSH)
	{
		log_info(logCliente, "	Recibi RESPUESTA_FLUSH");
	}
	else if (head == ENOENTRY)
	{
		log_info(logCliente, "	Recibi respuesta ENOENT en osada_flush........................................");
	}
	else
	{
		log_info(logCliente, "	No recibi RESPUESTA_FLUSH en osada_flush......................................");
	}

	free(paquete);
	return 0;
}

static int osada_getattr(const char *path, struct stat* stbuf)
{
	log_info(logCliente, "FUSE: llamada a osada_getattr(), path: %s", path);

	memset(stbuf, 0, sizeof(struct stat));

  if ((strcmp(path, "/.Trash") != 0) &&
	  (strcmp(path, "/.Trash-1000") != 0) &&
	  (strcmp(path, " /.xdg-volume-info") != 0) &&
	  (strcmp(path, "/autorun.inf") != 0) &&
	  (strcmp(path, "/.xdg-volume-info") != 0))
      {
		int head = 0;
		t_stbuf *paquete = NULL;
		char* pathAux = malloc(strlen(path) +1);
		strcpy(pathAux, path);

		log_info(logCliente, "	Envio PEDIDO_GETATTR");
		pthread_mutex_lock(&mutex_comunicacion);
			enviar(*socketServidor, PEDIDO_GETATTR, (void*)pathAux);
			paquete = (t_stbuf*)recibir(*socketServidor,&head);
		pthread_mutex_unlock(&mutex_comunicacion);

		if (head == RESPUESTA_GETATTR)
		{
			stbuf->st_mode = paquete->mode;
			stbuf->st_nlink = paquete->nlink;
			stbuf->st_size = paquete->size;

//		 	time_t tiempo = time(0);
//		 	time(&tiempo);
//		 	time_t timeinfo =  (time_t)localtime(&tiempo);
//		 	//printf(YEL "\t Local Time: %s\r\n" RESET, asctime(timeinfo));
//		 	return timeinfo;

			time_t tiempo =(time_t)paquete->mtime;

//			stbuf->st_mtim.tv_sec = (time_t)(tiempo/1000);
//			stbuf->st_mtim.tv_nsec = (tiempo % 1000) * 1000000;
//			stbuf->st_atim.tv_sec = (time_t)(tiempo/1000);
//			stbuf->st_atim.tv_nsec = (tiempo % 1000) * 1000000;
//			stbuf->st_ctim.tv_sec = (time_t)(tiempo/1000);
//			stbuf->st_ctim.tv_nsec = (tiempo % 1000) * 1000000;

			stbuf->st_mtim.tv_sec = tiempo;
			stbuf->st_atim.tv_sec = tiempo;
			stbuf->st_ctim.tv_sec = tiempo;

			//printf("\tctime: %s", ctime(&sb.st_ctim.tv_sec));
//			log_info(logCliente, "	fecha last mod copiada: %s",ctime(&(stbuf->st_ctim.tv_sec)));
//			log_info(logCliente, "	fecha last mod recibida en paquete: %s",ctime(&(paquete->mtime)));
//			log_info(logCliente, "	fecha last mod uint32: %d",paquete->mtime);
			log_info(logCliente, "	Recibi RESPUESTA_GETATTR");

			free(paquete);

			return 0;
		}
		else if(head == ENOENT)
		{
			log_info(logCliente, "	Recibi ENOENT");
			free(paquete);
			return -ENOENT;
		}
		else
		{
			log_info(logCliente, "	Recibi algo inesperado %d", head);
			free(paquete);
			return -ENOENT;
		}
	}
	else
	{
		log_info(logCliente, "	Recibi ENOENT");
		return -ENOENT;
	}
}

static int osada_mkdir(const char *path, mode_t mode)
{
	// crear un directorio
	log_info(logCliente, "FUSE: llamada a osada_mkdir(), path: %s", path);

	int head = 0;
	void* paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);

	log_info(logCliente, "	Envio PEDIDO_MKDIR");
	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_MKDIR, (void*)pathAux);
		paquete = recibir(*socketServidor, &head);
	pthread_mutex_unlock(&mutex_comunicacion);

	free(paquete);
	switch (head)
	{
		case RESPUESTA_MKDIR:
			log_info(logCliente, "	Recibi RESPUESTA_MKDIR");
			return 0;

		case ERRDQUOT:
			log_info(logCliente, "	Recibi respuesta EDQUOT en osada_mkdir");
			return -EDQUOT;

		case ERRNAMETOOLONG:
			log_info(logCliente, "	Recibi respuesta ENAMETOOLONG en osada_mkdir");
			return -ENAMETOOLONG;

		case ENOENTRY:
			log_info(logCliente, "	No recibi RESPUESTA_MKDIR en osada_mkdir");
			return 0;
	}
	return 0;
}

static int osada_mknod(const char *path, mode_t mode, dev_t rdev)
{
	// crear un archivo
	log_info(logCliente, "FUSE: llamada a osada_mknod(), path: %s", path);

	int head = 0;
	void* paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);

	log_info(logCliente, "	Envio PEDIDO_MKNOD");
	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_MKNOD, pathAux);
		paquete = recibir(*socketServidor, &head);
	pthread_mutex_unlock(&mutex_comunicacion);
	free(paquete);

	switch (head)
	{
		case RESPUESTA_MKNOD:
			log_info(logCliente, "	Recibi RESPUESTA_MKNOD");
			return 0;

		case ERRDQUOT:
			log_info(logCliente, "	Recibi respuesta EDQUOT en osada_mknod");
			return -EDQUOT;

		case ERRNAMETOOLONG:
			log_info(logCliente, "	Recibi respuesta ENAMETOOLONG en osada_mknod");
			return -ENAMETOOLONG;

		case ENOENTRY:
			log_info(logCliente, "	No recibi RESPUESTA_MKNOD en osada_mknod");
			return 0;
	}
	return 0;
}

//static int hello_open(const char *path, struct fuse_file_info *fi)
//{
//        if (strcmp(path, hello_path) != 0)
//                return -ENOENT;
//        if ((fi->flags & 3) != O_RDONLY)
//                return -EACCES;
//        return 0;
//}
static int osada_open(const char *path, struct fuse_file_info *fi)
{
	log_info(logCliente, "FUSE: llamada a osada_open(), path: %s", path);
	log_info(logCliente, "	flags: %d", fi->flags);
	log_info(logCliente, "	fh:");// %f");, fi->fh);


	int head = 0;
	void *paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);
	log_info(logCliente, "	Envio PEDIDO_OPEN");

	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_OPEN, (void*) pathAux);
		paquete = recibir(*socketServidor,&head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_OPEN)
	{
		log_info(logCliente, "	Recibi RESPUESTA_OPEN");

		free(paquete);
		return 0;
	}
	else
	{
		log_info(logCliente, "	Recibi respuesta ENOENT");
		free(paquete);
		return -ENOENT;
	}

	return 0;
}

static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	log_info(logCliente, "FUSE: llamada a osada_readdir(), path: %s", path);

	int head = 0;
	void *paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);

	log_info(logCliente, "	Envio PEDIDO_READDIR");
	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_READDIR, pathAux);
		paquete = recibir(*socketServidor,&head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_READDIR)
	{
		log_info(logCliente, "	Recibi RESPUESTA_READDIR");
		log_info(logCliente, (char*)paquete);

		char *token = NULL;

		token = strtok(paquete, "/");

		while (token != NULL)
		{
			filler(buf, token, NULL, 0);
			token = strtok(NULL, "/");
		}

		free(paquete);
		return 0;
	}
	else
	{
		log_info(logCliente, "	Recibi respuesta ENOENT");
		free(paquete);
		return -ENOENT;
	}
}

static int osada_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
 //leer un archivo abierto
	log_info(logCliente, "FUSE: llamada a osada_read(), path: %s", path);
	log_info(logCliente, "Size pedido: %d", size);
	log_info(logCliente, "Offset pedido: %d", offset);

	if ((strcmp(path, "/.Trash") != 0) && (strcmp(path, "/.Trash-1000") != 0) && (strcmp(path, " /.xdg-volume-info") != 0) && (strcmp(path, "/autorun.inf") != 0) && (strcmp(path, "/.xdg-volume-info") != 0))
	{
		int head = 0;
		uint32_t tamanio = 0;
		t_writebuf *pedidoAux = malloc(sizeof(t_readbuf));
		memset(pedidoAux, 0, sizeof(t_readbuf));

		t_readbuf *pedido = malloc(sizeof(t_readbuf));
		pedido->pathLen = strlen(path) + 1;
		pedido->size = size;
		pedido->offset = offset;

		void* paquete = NULL;
		char* pathAux = malloc(strlen(path) +1);
		strcpy(pathAux, path);

		log_info(logCliente, "	Envio PEDIDO_READ");
		pthread_mutex_lock(&mutex_comunicacion);
			enviar(*socketServidor, PEDIDO_READ, pedidoAux);
			enviarEstructuraRead(*socketServidor, PEDIDO_READ, pathAux, pedido);
			paquete = recibirRespuestaRead(*socketServidor, &head, &tamanio);
		pthread_mutex_unlock(&mutex_comunicacion);

		if (head == RESPUESTA_READ)
		{
			memset(buf, 0, tamanio);
			memcpy(buf, paquete, tamanio);
			log_info(logCliente, "	Recibi RESPUESTA_READ");
			free(paquete);
			return tamanio;
		}
		else if (head == ENOENTRY)
		{
			log_info(logCliente, "	Recibi respuesta ENOENT en osada_read");
		}
		else
		{
			log_info(logCliente, "	No recibi RESPUESTA_READ en osada_read");
		}
		free(paquete);
	}

	return size;
}

static int osada_release(const char *path, struct fuse_file_info *fi)
{
// libero un archivo que fue abierto antes
	log_info(logCliente, "FUSE: llamada a osada_release(), path: %s", path);

	int head = 0;
	void* paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);

	log_info(logCliente, "	Envio PEDIDO_RELEASE");
	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_RELEASE, pathAux);
		paquete = recibir(*socketServidor, &head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_RELEASE)
	{
		log_info(logCliente, "	Recibi RESPUESTA_RELEASE");
	}
	else if (head == ENOENTRY)
	{
		log_info(logCliente, "	Recibi respuesta ENOENT en osada_release");
	}
	else
	{
		log_info(logCliente, "	No recibi RESPUESTA_RELEASE en osada_release");
	}
	free(paquete);

	return 0;
}

static int osada_rename(const char *path, const char *newpath)
{
	// renombrar un archivo
	log_info(logCliente, "FUSE: llamada a osada_rename()" );
	log_info(logCliente, "path: %s", path);
	log_info(logCliente, "new path: %s",  newpath);

	int head = 0;
	void* paquete = NULL;

	void *pedido = malloc(strlen(path) + strlen(newpath) + 3);
	memset(pedido, 0, strlen(path) + strlen(newpath) + 3);

	char* str = malloc(strlen(path) + strlen(newpath) + 3);
	memset(str, 0, strlen(path) + strlen(newpath) + 3);
	str = strcpy(str, path);
	str = strcat(str, "*");
	str = strcat(str, newpath);
	memcpy(pedido, str, strlen(path) + strlen(newpath) + 3);
	free(str);

	log_info(logCliente, "	Envio PEDIDO_RENAME");
	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_RENAME, pedido);
		paquete = recibir(*socketServidor, &head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_RENAME)
	{
		log_info(logCliente, "	Recibi RESPUESTA_RENAME");
	}
	else if (head == ERRNAMETOOLONG)
	{
		log_info(logCliente, "	Recibi respuesta ERRNAMETOOLONG en osada_rename");
		free(paquete);
		return -ENAMETOOLONG;
	}
	else
	{
		log_info(logCliente, "	No recibi RESPUESTA_RENAME en osada_rename");
	}
	free(paquete);

	return 0;
}

static int osada_rmdir(const char *path)
{
	// borrar un directorio
	log_info(logCliente, "FUSE: llamada a osada_rmdir(), path: %s", path);

	int head = 0;
	void* paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);

	log_info(logCliente, "	Envio PEDIDO_RMDIR");
	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_RMDIR, pathAux);
		paquete = recibir(*socketServidor, &head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_RMDIR)
	{
		log_info(logCliente, "	Recibi RESPUESTA_RMDIR");
	}
	else if (head == ENOENTRY)
	{
		log_info(logCliente, "	Recibi respuesta ENOENT en osada_rmdir");
	}
	else
	{
		log_info(logCliente, "	No recibi RESPUESTA_RMDIR en osada_rmdir");
	}
	free(paquete);

	return 0;

}

static int osada_truncate(const char *path, off_t new_size)
{
	//tengo un archivo abierto y lo quiero redimensionar
	log_info(logCliente, "FUSE: llamada a osada_truncate(), path: %s", path);
	log_info(logCliente, "New size: %d", new_size);

	int head = 0;
	void* paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);
    off_t* newSize = malloc(sizeof(off_t));
    memset(newSize,0, sizeof(off_t));
    memcpy(newSize, &new_size, sizeof(off_t));

    log_info(logCliente, "	Envio PEDIDO_TRUNCATE");
	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_TRUNCATE, pathAux);
		enviar(*socketServidor, PEDIDO_TRUNCATE_NEW_SIZE, newSize);
		paquete =  recibir(*socketServidor,&head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_TRUNCATE)
	{
		log_info(logCliente, "	Recibi RESPUESTA_TRUNCATE");
	}
	else if (head == ENOENTRY)
	{
		log_info(logCliente, "	Recibi respuesta ENOENT en osada_truncate");
	}
	else
	{
		log_info(logCliente, "	No recibi RESPUESTA_TRUNCATE en osada_truncate");
	}
	free(paquete);

	return 0;
}

static int osada_unlink(const char *path)
{
	// borrar un archivo
	log_info(logCliente, "FUSE: llamada a osada_unlink(), path: %s", path);

	int head = 0;
	void* paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);
	log_info(logCliente, "	Envio PEDIDO_UNLINK");

	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_UNLINK, pathAux);
		paquete =  recibir(*socketServidor, &head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_UNLINK)
	{
		log_info(logCliente, "	Recibi RESPUESTA_UNLINK");
	}
	else if (head == ENOENTRY)
	{
		log_info(logCliente, "	Recibi respuesta ENOENT en osada_unlink");
	}
	else
	{
		log_info(logCliente, "	No recibi RESPUESTA_UNLINK en osada_unlink");
	}
	free(paquete);

	return 0;
}

static int osada_utimens(const char* path, const struct timespec ts[2])
{
	// actualizar la informacion de ultimo acceso a un archivo
	log_info(logCliente, "FUSE: llamada a osada_utimens(), path: %s", path);

	int head = 0;
	void* paquete = NULL;
	char* pathAux = malloc(strlen(path) +1);
	strcpy(pathAux, path);
	log_info(logCliente, "	Envio PEDIDO_UTIMENS");

	pthread_mutex_lock(&mutex_comunicacion);
		enviar(*socketServidor, PEDIDO_UTIMENS, pathAux);
		paquete =  recibir(*socketServidor, &head);
	pthread_mutex_unlock(&mutex_comunicacion);

	if (head == RESPUESTA_UTIMENS)
	{
		log_info(logCliente, "	Recibi RESPUESTA_UTIMENS");
	}
	else
	{
		log_info(logCliente, "	Recibi algo");
	}

	free(paquete);
	return 0;
}

static int osada_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	// escribir un archivo
	log_info(logCliente, "FUSE: llamada a osada_write(), path: %s", path);
	log_info(logCliente, "Size: %d", size);

	if ((strcmp(path, "/.Trash") != 0) && (strcmp(path, "/.Trash-1000") != 0) && (strcmp(path, " /.xdg-volume-info") != 0) && (strcmp(path, "/autorun.inf") != 0) && (strcmp(path, "/.xdg-volume-info") != 0))
	{
		int head = 0;

		t_writebuf *pedidoAux = malloc(sizeof(t_writebuf));
		memset(pedidoAux,0, sizeof(t_writebuf));

		t_writebuf *pedido = malloc(sizeof(t_writebuf));
		memset(pedido,0, sizeof(t_writebuf));
		pedido->pathLen = strlen(path) + 1;
		pedido->bufLen = size +1;
		pedido->size = size;
		pedido->offset = offset;

		char* pathEnviar = malloc(strlen(path) +1);
		char* bufEnviar = malloc(size +1);
		memset(pathEnviar, 0, strlen(path) +1);
		memset(bufEnviar, 0, size +1);
		memcpy(pathEnviar, path, strlen(path) +1);
		memcpy(bufEnviar, buf, size);

		log_info(logCliente, "bufLen: %d", pedido->bufLen);

		uint32_t* tamanio = NULL;

		log_info(logCliente, "	Envio PEDIDO_WRITE");
		pthread_mutex_lock(&mutex_comunicacion);
			enviar(*socketServidor, PEDIDO_WRITE, pedidoAux);
		    enviarEstructuraWrite(*socketServidor, PEDIDO_WRITE, pathEnviar, bufEnviar, pedido);
			tamanio = (uint32_t*)recibir(*socketServidor,&head);
		pthread_mutex_unlock(&mutex_comunicacion);

		if (head == RESPUESTA_WRITE)
		{
			log_info(logCliente, "	tamanio: %d", *tamanio);
			log_info(logCliente, "	Recibi RESPUESTA_WRITE");
			free(tamanio);
		}
		else if (head == ERRNOSPC)
		{
			log_info(logCliente, "	Recibi respuesta ENOSPC en osada_write");
			free(tamanio);
			return -ENOSPC;//no hay espacio en disco
		}
		else if (head == ERRFBIG)
		{
			log_info(logCliente, "	Recibi respuesta ERRFBIG en osada_write");
			free(tamanio);
			return -EFBIG;
		}
		else if (head == ENOENTRY)
		{
			log_info(logCliente, "	Recibi respuesta ENOENTRY en osada_write");
			free(tamanio);
			return -ENOENT;
		}
		else
		{
			free(tamanio);
		}
	}
	return size;
}

//funciones para imprimir estados en log-------------------------------------------------------------------------------
void printConectado()
{
	printf("\t Recibi mensaje handshake  del servidor\n\n");
	log_info(logCliente, "	Recibi mensaje handshake  del servidor");
	printf( "****************** " GRN "Se establecio la conexion con exito " RESET "***************************\n");
	log_info(logCliente, "****************** Se establecio la conexion con exito *********************" );
}

void printEncabezado()
{
	printf(GRN "\n\n\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" RESET);
	printf("**********************************************************************************\n");
	printf("****************** " GRN "CLIENTE POKEDEX " RESET "***********************************************\n");
	printf("**********************************************************************************\n\n\n");
	printf("****************** Iniciando cliente...\n\n");
	printf("****************** Creando archivo Log *******************************************\n");
}

void printErrorConexion()
{
	printf(RED "\n\n\n****************** No se pudo establecer la conexion con el servidor *************\n" RESET);
	printf("****************** Terminando el programa ****************************************\n");
	printf("**********************************************************************************\n\n\n");
	log_info(logCliente, "****************** No se pudo establecer la conexion con el servidor *************" );
	log_info(logCliente, "****************** Terminando el programa ****************************************" );
	log_info(logCliente, "**********************************************************************************" );
}

void PrintFuse()
{
	printf( "****************** " GRN "Llamo a fuse main" RESET "  ********************************************\n");
	log_info(logCliente, "****************** Llamo a fuse main  ********************************************" );
	printf("**********************************************************************************\n\n\n");
}

void printMensajeInesperado(int head)
{
	printf(YEL "\t Recibi mensaje: %d  del servidor\n\n" RESET, head);
	log_info(logCliente, "	Recibi mensaje inesperado  del servidor");
}

void printServidorDesconectado()
{
	printf(RED "\n\n****************** El servidor cerro la conexion *********************************\n" RESET);
	printf(NAR"**********************************************************************************\n");
	printf(ORG"****************** El cliente aborta el programa *********************************\n");
	printf(YEL"**********************************************************************************\n");
	printf(YEL2"****************** " GRN "Terminar" YEL2 " ******************************************************\n");
	printf(AMB"**********************************************************************************\n");
	printf(GRN "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n" RESET);


	log_info(logCliente, "****************** No se pudo establecer la conexion con el servidor *************" );
	log_info(logCliente, "****************** Terminando el programa ****************************************" );
	log_info(logCliente, "**********************************************************************************" );
}

void printTerminar()
{
	printf(NAR"\n\n**********************************************************************************\n");
	printf(ORG"****************** El cliente aborta la conexion *********************************\n");
	printf(YEL"**********************************************************************************\n");
	printf(YEL2"****************** " GRN "Terminar" YEL2 " ******************************************************\n");
	printf(AMB"**********************************************************************************\n");
	printf(GRN "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n" RESET);
}

void terminar()
{

	printf("Señal terminar");
	exit(0);
}
