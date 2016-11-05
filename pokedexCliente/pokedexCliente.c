
/*
 * pokedexCliente.c
 *
 *  Created on: 17/9/2016
 *      Author: utnso
 */
#include <comunicacion.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <netdb.h>

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>

#include "pokedexCliente.h"

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

int* socketServidor;
t_log* logCliente;

char* puntoDeMontaje;
char mensaje[6] = "\0";
//char* ip;
//char* puerto;

char ip[10];
char puerto[5];

int iret1;
pthread_t thread_Planificador;
pthread_mutex_t mutex_comunicacion  = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[])
{
//	ip = getenv("SERVER_HOST");
//	puerto = getenv("SERVER_PORT");
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  int head=0;
  void *mensajeHSK = NULL;

  printEncabezado();

  logCliente = log_create("logPokedex", "Pokedex", false, LOG_LEVEL_DEBUG);
  log_info(logCliente, "****************** Creando archivo Log *******************************************" );

  memset(ip,0,10);
  memset(puerto,0,5);
  memset(mensaje,0,5);

  strcpy(ip,"127.0.0.1");
  strcpy(puerto,"4969");
  strcpy(mensaje,"sock");

  signal(SIGINT,terminar);
  signal(SIGTERM,terminar);

  //pthread_mutex_lock(&mutex_comunicacion);
  socketServidor = malloc(sizeof(int*));

  printf("****************** Estableciendo conexion con el servidor... \n\n");
  log_info(logCliente, "****************** Estableciendo conexion con el servidor... " );

  int socket = crearSocket(ip, puerto);

  if (socket == -1)
  {
    printErrorConexion();
    return -1;
  }
  else if (socket == -2)
  {
    printServidorDesconectado();
    return -1;
  }

  memset(socketServidor,0,sizeof(int*));
  *socketServidor = socket;
  printf("\t Enviando mensaje handshake al servidor \n");
  log_info(logCliente, "	Enviando mensaje handshake al servidor" );

  enviar(*socketServidor, HANDSHAKE, mensaje);
  mensajeHSK = recibir(*socketServidor,&head);
  //pthread_mutex_unlock(&mutex_comunicacion);

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
  return fuse_main(args.argc, args.argv, &osada_oper, NULL);;
}

//funciones para gestionar los pedidos de FUSE---------------------------------------------------------------------------------
static int osada_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
  // crear y abrir un archivo
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_create() ****************************" );
  log_info(logCliente, "******************************************************************************" );

  log_info(logCliente, path);

  int head = 0;
  void* paquete = NULL;

  pthread_mutex_lock(&mutex_comunicacion);
    enviar(*socketServidor, PEDIDO_CREATE, path);
  //	log_info(logCliente, "	Envie PEDIDO_CREATE");
    paquete = recibir(*socketServidor, &head);
  pthread_mutex_unlock(&mutex_comunicacion);

  if (head == RESPUESTA_CREATE)
  {
    log_info(logCliente, "	Recibi RESPUESTA_CREATE");
  }
  else if (head == ENOENTRY)
  {
    log_info(logCliente, "	Recibi respuesta ENOENT en osada_create....... ........ ..... ...... ..... .... .....");
  }
  else
  {
    log_info(logCliente, "	No recibi RESPUESTA_CREATE en osada_create.... ....... ........ ....... ........ ..... ....");
  }
  free(paquete);

  return 0;
}

static int osada_getattr(const char *path, struct stat *stbuf)
{
  log_info(logCliente, "****************** FUSE: llamada a osada_getattr() ******************************" );
  log_info(logCliente, path);

  memset(stbuf, 0, sizeof(struct stat));

  if ((strcmp(path, "/.Trash") != 0) && (strcmp(path, "/.Trash-1000") != 0) && (strcmp(path, " /.xdg-volume-info") != 0) && (strcmp(path, "/autorun.inf") != 0) && (strcmp(path, "/.xdg-volume-info") != 0))
  {
    int head = 0;
    t_stbuf *paquete = NULL;

    pthread_mutex_lock(&mutex_comunicacion);
      enviar(*socketServidor, PEDIDO_GETATTR, path);
      //log_info(logCliente, "	Envie PEDIDO_GETATTR");
      paquete = (t_stbuf*)recibir(*socketServidor,&head);
    pthread_mutex_unlock(&mutex_comunicacion);

    if (head == RESPUESTA_GETATTR)
    {
      stbuf->st_mode = paquete->mode;
      stbuf->st_nlink = paquete->nlink;
      stbuf->st_size = paquete->size;

      log_info(logCliente, "	Recibi RESPUESTA_GETATTR");
      return 0;
    }
    else if(head == ENOENT)
    {
      log_info(logCliente, "	Recibi ENOENT");
      return -ENOENT;
    }
    else
    {
      log_info(logCliente, "	Recibi algo inesperado %d", head);
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
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_mkdir() *****************************" );
  log_info(logCliente, "******************************************************************************" );

  log_info(logCliente, path);

  int head = 0;
  void* paquete = NULL;

  pthread_mutex_lock(&mutex_comunicacion);
    enviar(*socketServidor, PEDIDO_MKDIR, path);
    log_info(logCliente, "	Envie PEDIDO_MKDIR");
    paquete = recibir(*socketServidor, &head);
  pthread_mutex_unlock(&mutex_comunicacion);

  if (head == RESPUESTA_MKDIR)
  {
    log_info(logCliente, "	Recibi RESPUESTA_MKDIR");
  }
  else if (head == ENOENTRY)
  {
    log_info(logCliente, "	Recibi respuesta ENOENT en osada_mkdir....... ........ ..... ...... ..... .... .....");
  }
  else
  {
    log_info(logCliente, "	No recibi RESPUESTA_MKDIR en osada_mkdir.... ....... ........ ....... ........ ..... ....");
  }
  free(paquete);

  return 0;
}

static int osada_open(const char *path, struct fuse_file_info *fi)
{
  //TODO: enviar al server el path y chequear que el archivo existe, ver de tener una tabla de archivos abiertos y si es para escritura o lectura
  log_info(logCliente, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" );
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_open() ******************************" );
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" );
  log_info(logCliente, path);

  int head = 0;
  void *paquete = NULL;

  pthread_mutex_lock(&mutex_comunicacion);
    enviar(*socketServidor, PEDIDO_OPEN, path);
    log_info(logCliente, "	Envie PEDIDO_OPEN");
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
    return -ENOENT;
  }

  return 0;
}

static int osada_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_readdir() ***************************" );
  log_info(logCliente, "******************************************************************************" );

  log_info(logCliente, path);

  int head = 0;
  void *paquete = NULL;

  pthread_mutex_lock(&mutex_comunicacion);
    enviar(*socketServidor, PEDIDO_READDIR, path);
    log_info(logCliente, "	Envie PEDIDO_READDIR");
    paquete = recibir(*socketServidor,&head);
  pthread_mutex_unlock(&mutex_comunicacion);

  if (head == RESPUESTA_READDIR)
  {
    log_info(logCliente, "	Recibi RESPUESTA_READDIR");
    log_info(logCliente, (char*)paquete);

    char *token = malloc(strlen(paquete)+1);

    token = strtok(paquete, "/");

    while (token != NULL)
    {
      filler(buf, token, NULL, 0);

      log_info(logCliente, (char*)token);
      token = strtok(NULL, "/");
    }

    free(token);
    free(paquete);
    return 0;
  }
  else
  {
    log_info(logCliente, "	Recibi respuesta ENOENT");
    return -ENOENT;
  }
}

static int osada_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
 //leer un archivo abierto
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_read() ******************************" );
  log_info(logCliente, "******************************************************************************" );

  log_info(logCliente, path);
  log_info(logCliente, "Size pedido: %d", size);
  log_info(logCliente, "Offset pedido: %d", offset);

  if ((strcmp(path, "/.Trash") != 0) && (strcmp(path, "/.Trash-1000") != 0) && (strcmp(path, " /.xdg-volume-info") != 0) && (strcmp(path, "/autorun.inf") != 0) && (strcmp(path, "/.xdg-volume-info") != 0))
  {
    int head = 0;
    uint32_t tamanio = 0;
    t_readbuf *pedido = malloc(sizeof(t_readbuf));
    pedido->pathLen = strlen(path) + 1;
    pedido->size = size;
    pedido->offset = offset;

    void* paquete = NULL;

    pthread_mutex_lock(&mutex_comunicacion);
      enviar(*socketServidor, PEDIDO_READ, pedido);
      enviarEstructuraRead(*socketServidor, PEDIDO_READ, path, pedido);
      log_info(logCliente, "	Envie PEDIDO_READ");

      paquete = recibirRespuestaRead(*socketServidor, &head, &tamanio);

    pthread_mutex_unlock(&mutex_comunicacion);
    //free(pedido);

    if (head == RESPUESTA_READ)
    {
      memset(buf, 0, tamanio);
      memcpy(buf, paquete, tamanio);
      log_info(logCliente, "Size recibido: %d", tamanio);
      log_info(logCliente, "	Recibi RESPUESTA_READ");
      return tamanio;
    }
    else if (head == ENOENTRY)
    {
      log_info(logCliente, "	Recibi respuesta ENOENT en osada_read....... ........ ..... ...... ..... .... .....");
    }
    else
    {
      log_info(logCliente, "	No recibi RESPUESTA_READ en osada_read.... ....... ........ ....... ........ ..... ....");
    }
    free(paquete);
  }

  return size;
}


static int osada_rename(const char *path, const char *newpath)
{
  // renombrar un archivo
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_rename() ****************************" );
  log_info(logCliente, "******************************************************************************" );

  log_info(logCliente, "path: %s", path);
  log_info(logCliente, "new path: %s",  newpath);

  int head = 0;
  void* paquete = NULL;
  int offset = 0;
  char* centinela = malloc(sizeof(char));
  centinela[0] = "*";
  void *pedido = malloc(strlen(path) + strlen(newpath) + 3);
  memset(pedido, 0, strlen(path) + strlen(newpath) + 3);

  char* str = malloc(strlen(path) + strlen(newpath) + 3);
  memset(str, 0, strlen(path) + strlen(newpath) + 3);
  strcpy(str, path);
  strcat(str, "*");
  strcat(str, newpath);
  memcpy(pedido, str, strlen(path) + strlen(newpath) + 3);


  pthread_mutex_lock(&mutex_comunicacion);
    enviar(*socketServidor, PEDIDO_RENAME, pedido);
    log_info(logCliente, "	Envie PEDIDO_RENAME");
    paquete = recibir(*socketServidor, &head);
  pthread_mutex_unlock(&mutex_comunicacion);

  if (head == RESPUESTA_RENAME)
  {
    log_info(logCliente, "	Recibi RESPUESTA_RENAME");
  }
  else if (head == ENOENTRY)
  {
    log_info(logCliente, "	Recibi respuesta ENOENT en osada_rename....... ........ ..... ...... ..... .... .....");
  }
  else
  {
    log_info(logCliente, "	No recibi RESPUESTA_RENAME en osada_rename.... ....... ........ ....... ........ ..... ....");
  }
  free(paquete);

  return 0;
}

static int osada_rmdir(const char *path)
{
  // borrar un directorio
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_rmdir() *****************************" );
  log_info(logCliente, "******************************************************************************" );

  log_info(logCliente, path);

  int head = 0;
  void* paquete = NULL;

  pthread_mutex_lock(&mutex_comunicacion);
    enviar(*socketServidor, PEDIDO_RMDIR, path);
    log_info(logCliente, "	Envie PEDIDO_RMDIR");
    paquete = recibir(*socketServidor, &head);
  pthread_mutex_unlock(&mutex_comunicacion);

  if (head == RESPUESTA_RMDIR)
  {
    log_info(logCliente, "	Recibi RESPUESTA_RMDIR");
  }
  else if (head == ENOENTRY)
  {
    log_info(logCliente, "	Recibi respuesta ENOENT en osada_rmdir....... ........ ..... ...... ..... .... .....");
  }
  else
  {
    log_info(logCliente, "	No recibi RESPUESTA_RMDIR en osada_rmdir.... ....... ........ ....... ........ ..... ....");
  }
  free(paquete);

  return 0;

}

static int osada_truncate(const char *path, off_t new_size)
{
  return 0;
  /* Truncate or extend the given file so that it is precisely size bytes long.
   * This call is required for read/write filesystems, because recreating a file will first truncate it.*/

  //tengo un archivo abierto y lo quiero redimensionar
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_truncate() **************************" );
  log_info(logCliente, "******************************************************************************" );

  log_info(logCliente, path);
  log_info(logCliente, "New size: %d", new_size);

  int head = 0;
  void* paquete = NULL;

  pthread_mutex_lock(&mutex_comunicacion);
    enviar(*socketServidor, PEDIDO_TRUNCATE, path);
    log_info(logCliente, "	Envie PEDIDO_TRUNCATE");
    paquete =  recibir(*socketServidor,&head);
  pthread_mutex_unlock(&mutex_comunicacion);

  if (head == RESPUESTA_TRUNCATE)
  {
    log_info(logCliente, "	Recibi RESPUESTA_TRUNCATE");
    //new_size = *paquete; //ver si new size viene con algo o lo tengo que llenar
  }
  else if (head == ENOENTRY)
  {
    log_info(logCliente, "	Recibi respuesta ENOENT en osada_truncate....... ........ ..... ...... ..... .... .....");
  }
  else
  {
    log_info(logCliente, "	No recibi RESPUESTA_TRUNCATE en osada_truncate.... ....... ........ ....... ........ ..... ....");
  }
  free(paquete);

  return 0;
}

static int osada_unlink(const char *path)
{
  // borrar un archivo
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_unlink() ****************************" );
  log_info(logCliente, "******************************************************************************" );

  log_info(logCliente, path);

  int head = 0;
  void* paquete = NULL;

  pthread_mutex_lock(&mutex_comunicacion);
    enviar(*socketServidor, PEDIDO_UNLINK, path);
    log_info(logCliente, "	Envie PEDIDO_UNLINK");
    paquete =  recibir(*socketServidor, &head);
  pthread_mutex_unlock(&mutex_comunicacion);

  if (head == RESPUESTA_UNLINK)
  {
    log_info(logCliente, "	Recibi RESPUESTA_UNLINK");
  }
  else if (head == ENOENTRY)
  {
    log_info(logCliente, "	Recibi respuesta ENOENT en osada_unlink....... ........ ..... ...... ..... .... .....");
  }
  else
  {
    log_info(logCliente, "	No recibi RESPUESTA_UNLINK en osada_unlink.... ....... ........ ....... ........ ..... ....");
  }
  free(paquete);

  return 0;
}

static int osada_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  // escribir un archivo
  log_info(logCliente, "******************************************************************************" );
  log_info(logCliente, "****************** FUSE: llamada a osada_write() *****************************" );
  log_info(logCliente, "******************************************************************************" );

  log_info(logCliente, path);
  log_info(logCliente, "Size: %d", size);

  if ((strcmp(path, "/.Trash") != 0) && (strcmp(path, "/.Trash-1000") != 0) && (strcmp(path, " /.xdg-volume-info") != 0) && (strcmp(path, "/autorun.inf") != 0) && (strcmp(path, "/.xdg-volume-info") != 0))
  {
    int head = 0;

    t_writebuf *pedido = malloc(sizeof(t_writebuf));
    pedido->pathLen = strlen(path) + 1;
    pedido->bufLen = strlen(buf) + 1;
    pedido->size = size;
    pedido->offset = offset;

    uint32_t* tamanio = 0;

    pthread_mutex_lock(&mutex_comunicacion);
      enviar(*socketServidor, PEDIDO_WRITE, pedido);
        enviarEstructuraWrite(*socketServidor, PEDIDO_WRITE, path, buf, pedido);
      log_info(logCliente, "	Envie PEDIDO_WRITE");
      tamanio = (uint32_t*)recibir(*socketServidor,&head);
    pthread_mutex_unlock(&mutex_comunicacion);
    //free(pedido);

    if (head == RESPUESTA_WRITE)
    {
      log_info(logCliente, "	Recibi RESPUESTA_WRITE");
      return *tamanio;
    }
    else if (head == ENOENTRY)
    {
      log_info(logCliente, "	Recibi respuesta ENOENT en osada_write....... ........ ..... ...... ..... .... .....");
      return ENOSPC;//no hay espacio en disco
    }
    else
    {
      log_info(logCliente, "	No recibi RESPUESTA_WRITE en osada_white.... ....... ........ ....... ........ ..... ....");
      return ENOSPC;
    }
    //free(tamanio);
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
  printf(RED "\n\n\n****************** El servidor cerro la conexion *********************************\n" RESET);
  printf("****************** Terminando el programa ****************************************\n");
  printf("**********************************************************************************\n\n\n");
  log_info(logCliente, "****************** No se pudo establecer la conexion con el servidor *************" );
  log_info(logCliente, "****************** Terminando el programa ****************************************" );
  log_info(logCliente, "**********************************************************************************" );
}

void terminar()
{
  printf("Señal terminar");
  exit(0);
}
