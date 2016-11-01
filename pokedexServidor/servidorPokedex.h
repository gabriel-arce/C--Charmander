/*
 * servidorPokedex.h
 *
 *  Created on: 5/10/2016
 *      Author: utnso
 */

#ifndef SERVIDORPOKEDEX_H_
#define SERVIDORPOKEDEX_H_

#define PUERTO "4969"
#define BACKLOG 15	// Define cuantas conexiones vamos a mantener pendientes al mismo tiempo
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

#define PEDIDO_OPEN 33
#define RESPUESTA_OPEN 34

//funciones de disco--------------------------
void asignarOffsets();
void descargar(uint32_t descriptorArchivo);
void escribirArchivo(uint32_t posicion, char* buf);
void escribirAsignacion(uint32_t posicion, int buf);


void escribirBloque(uint32_t bloque, char* buf);
int existePath(char* path, int* pos);
int existeDirectorio(unsigned char* token, uint16_t* padre, int* posicion);
void inicializarDisco();
void leerArchivo(uint32_t posicion, osada_file* buf);
void leerAsignacion(uint32_t posicion, osada_file* buf);
void leerBloque(uint32_t cantidadBloques, char* buf);
void leerDato(uint32_t posicion, osada_block* buf);
void leerHeader();
void leerTablaArchivos();
void leerTablaAsignaciones();
void leerTablaDatos();
int mapearDisco(char* path);
void mostrarHeader(osada_header oheader);

//nuevas de disco-----------------------------------
char abrirArchivo(char* path);
int agregarArchivo(char* path, int modo);
char borrarArchivo(char* path);
char borrarDirectorio(char* path);
osada_file* buscarArchivo(char* nombre, int* posicion);
char crearArchivo(char* path, int modo);
void* getAttr(char* path);
char* nombre(char* path);
int posicionUltimoToken(char* path);
void* readdir(char* path);
char renombrarArchivo(char* paths);
void* readFile(osada_file* archivo);
char writeFile(char* path, size_t size, void* bufWrite);

//funciones que faltan para write---------------------------------------
int proximaPosicionLibre();
int buscarEspacioLibre();//esta me parece que esta demas
int buscarBitLibre() ;//esta me parece que esta demas
int hayEspacioEnDisco(int cantidadBloques);

//funciones de servidor-------------------------------------------------
void atendercliente(int socket);
void* hiloComunicacion(void* arg);
void printEncabezado();
void printTerminar();

void* procesarPedidoCreate(char *pedido);
void* procesarPedidoGetatrr(char *path);
void* procesarPedidoMkdir(char *path);
void* procesarPedidoOpen(char* path);
void* procesarPedidoRead(void* buffer, uint32_t* tamanioBuffer);
void* procesarPedidoReaddir(char *path);
void* procesarPedidoRename(char *paths);
void* procesarPedidoRmdir(char *path);
void* procesarPedidoTruncate(char *path);
void* procesarPedidoUnlink(char *path);
void* procesarPedidoWrite(void *buffer);

void liberarRecursos();
void terminar();

#endif /* SERVIDORPOKEDEX_H_ */
