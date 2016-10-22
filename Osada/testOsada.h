/*
 * osada.h
 *
 *  Created on: 30 ago. 2016
 *      Author: gabriel
 */

#ifndef TESTOSADA_H_
#define TESTOSADA_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>
#include <commons/bitarray.h>
#include <fcntl.h>

#define BLOCK_SIZE										64
#define FILETABLE											1024
#define OSADA_FILENAME_LENGTH	17
#define MAX_FILES											2048
#define HEADER_BLOCK								1


#define LECTURA	"r"
#define ESCRITURA	"w"
#define EJECUCION	"x"
#define DIRECTORIO	"d"
#define NADA	"-"

typedef uint32_t osada_block_pointer;

typedef enum{
	CantidadArgumentosIncorrecta,
	NoSePudoAbrirIn,
	ErrorEnLectura,
	ErrorEnEscritura,
	ErrorStat,
	ErrorMmap,
	OtroError,
} Error;

typedef enum{
	InformacionGeneral,
	Header,
	Bitmap,
	TablaArchivos,
	TablaAsignaciones,
	BloqueDeDatos,
	InformacionFS,
} Titulo;

typedef enum{
	Falso,
	Verdadero,
} Bool;

typedef struct{
	unsigned char identificador[7];
	uint8_t	version;
	uint32_t bloquesFS;
	uint32_t bloquesBitMap;
	uint32_t inicioTablaAsignacion;
	uint32_t bloquesDatos;
	unsigned char relleno[40];
} t_osada_hdr;

typedef enum __attribute__((packed)) {
    DELETED = '\0',
    REGULAR = '\1',
    DIRECTORY = '\2',
} osada_file_state;

typedef struct {
	osada_file_state state;
	unsigned char fname[OSADA_FILENAME_LENGTH];
	uint16_t parent_directory;
	uint32_t file_size;
	uint32_t lastmod;
	osada_block_pointer first_block;
} t_osada_file;

typedef struct {
	unsigned int bit : 1;
} Bit;

//FILE *in;
int in;

char *hdr;
struct stat fileStat;
t_bitarray *bitVector;
uint32_t  bitmapSize, assignTable, dataBlocks, bloques, maximoBit;


t_osada_file *osada_filetable[FILETABLE];
t_osada_file *osada_file;
t_osada_hdr osada_header;
char *osada_bitmap;
int *osada_assignTable;
int *osada_blockData;



int *asignaciones;
char *datos;

void mostrarAyuda();
void mostrarMensajeDeError( Error );
void titulo(Titulo titulo);
void mostrarInfo();
void mostrarInfoFS();
void mostrarPermisos();
void levantarDatosGenerales();
void mapearArchivo();
void levantarHeader();
void levantarBitmap();
void levantarTablaArchivos();
void levantarTablaAsignaciones();
void levantarBloquesDeDatos();

#endif /* TESTOSADA_H_ */
