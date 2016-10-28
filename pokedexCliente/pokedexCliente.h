/*
 * pokedexCliente.h

 *
 *  Created on: 5/10/2016
 *      Author: guadalupe
 */

#ifndef POKEDEXCLIENTE_H_
#define POKEDEXCLIENTE_H_

#define CUSTOM_FUSE_OPT_KEY(t, p, v) { t, offsetof(struct t_runtime_options, p), v }

//pasar a un .h los pedidos respuestas
#define HANDSHAKE 777

#define PEDIDO_GETATTR 12
#define PEDIDO_READDIR 13
#define PEDIDO_TRUNCATE 14
#define PEDIDO_READ 15
#define PEDIDO_WRITE 16
#define PEDIDO_UNLINK 17
#define PEDIDO_MKDIR 18
#define PEDIDO_RMDIR 19
#define PEDIDO_RENAME 20
#define PEDIDO_CREATE 21

#define RESPUESTA_GETATTR 22
#define RESPUESTA_READDIR 23
#define RESPUESTA_TRUNCATE 24
#define RESPUESTA_READ 25
#define RESPUESTA_WRITE 26
#define RESPUESTA_UNLINK 27
#define RESPUESTA_MKDIR 28
#define RESPUESTA_RMDIR 29
#define RESPUESTA_RENAME 30
#define RESPUESTA_CREATE 31
#define ENOENTRY 32

static int osada_create(const char *path, mode_t mode, struct fuse_file_info *fi);
static int osada_getattr(const char *path, struct stat *stbuf);
static int osada_mkdir(const char *path, mode_t mode);
static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);
static int osada_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
static int osada_rename(const char *path, const char *newpath);
static int osada_rmdir(const char *path);
static int osada_truncate(const char *path, off_t new_size);
static int osada_unlink(const char *path);
static int osada_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

void printConectado();
void printEncabezado();
void printErrorConexion();
void PrintFuse();
void printMensajeInesperado(int head);
void printServidorDesconectado();

void terminar();

struct t_runtime_options
{
	char* welcome_msg;
} runtime_options;

static struct fuse_operations osada_oper = {
.getattr = osada_getattr,	// obtener atributos
.readdir = osada_readdir, // leer un directorio
.read = osada_read, // leer archivo
.write = osada_write, // escribir un archivo
.unlink = osada_unlink, // borrar un archivo
.mkdir = osada_mkdir,	// crear un directorio
.rmdir = osada_rmdir,	//borrar un directorio
.rename = osada_rename,	//renombrar un archivo
.create = osada_create, //crear y abrir un archivo
.truncate = osada_truncate, //redimensionar archivo
};

enum {
KEY_VERSION,
KEY_HELP,
};

static struct fuse_opt fuse_options[] = {
	// Este es un parametro definido por nosotros
	CUSTOM_FUSE_OPT_KEY("--welcome-msg %s", welcome_msg, 0),
	// Estos son parametros por defecto que ya tiene FUSE
	FUSE_OPT_KEY("-V", KEY_VERSION),
	FUSE_OPT_KEY("--version", KEY_VERSION),
	FUSE_OPT_KEY("-h", KEY_HELP),
	FUSE_OPT_KEY("--help", KEY_HELP),
	FUSE_OPT_END,
};
#endif /* POKEDEXCLIENTE_H_ */
