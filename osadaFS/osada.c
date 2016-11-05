#include <commons/bitarray.h>
#include <commons/string.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <comunicacion.h>

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include "osada.h"

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

char* disco;
off_t tamanioArchivo;
int32_t descriptorArchivo;
osada_header oheader;

int offsetBitmap;
int offsetTablaArchivos;
int offsetAsignaciones;
int offsetDatos;
int baseTablaAsignacionesBitmap;
int limiteTablaAsignacionesBitmap;

uint32_t  bitmapSize, dataBlocks, bloques, maximoBit;
t_bitarray* bitVector;
struct stat fileStat;

char abrirArchivo(char* path)
{
	//TODO: chequear que el archivo exista, ver de tener una tabla con archivos abiertos y el modo (lectura o escritura)
	return 's';
}

int agregarArchivo(char* path, int modo)
{
	//printf(CYN "\t nombre(path) en agregarArchivo %s\n" RESET, nombre(path));
	int pos;

	if(strcmp(nombre(path), ".Trash-1000") == 0)
	{
		return -1;
	}
	if(strlen(nombre(path)) > 17)
	{
		printf(RED "\t El nombre de archivo " YEL "%s" RED " supera el limite de caracteres maximo (17)\n" RESET, nombre(path));
		return -1;
	}
	if ((pos = buscarEspacioLibre()) == -1)// chequeo que exista espacio en la tabla de archivos, si hay agrego el pedido
	{
		printf(RED "\t No hay espacio para crear el archivo %s\n" RESET, nombre(path));
		return -1;
	}

	int p = padre(path);
	if (p == -1)
	{
		printf(RED "\t Salgo sin agregar el archivo %s\n" RESET, nombre(path));
		return -1;
	}
	osada_file* nuevo = malloc(sizeof(osada_file));
	memset(nuevo, 0, sizeof(osada_file));
	strcpy((char*)nuevo->fname, nombre(path));
	nuevo->file_size = 0;
	nuevo->lastmod = 0;//ver de poner la fecha con time()?
	nuevo->first_block = 0;//ver con que inicializar esto
	nuevo->state = modo;//si es archivo o directorio
	nuevo->parent_directory = p;
	escribirArchivo(pos, nuevo);

	return 0;
}

void asignarOffsets()
{
	int tamanioTablaAsig = (oheader.fs_blocks - 1025 - oheader.bitmap_blocks - oheader.data_blocks) * OSADA_BLOCK_SIZE;
	offsetBitmap = OSADA_BLOCK_SIZE;
	offsetTablaArchivos = OSADA_BLOCK_SIZE + (oheader.bitmap_blocks * OSADA_BLOCK_SIZE);
	offsetAsignaciones = offsetTablaArchivos + (1024 * OSADA_BLOCK_SIZE);
	offsetDatos = offsetAsignaciones + tamanioTablaAsig;

	bloques	= oheader.fs_blocks;// fileStat.st_size / OSADA_BLOCK_SIZE;
	bitmapSize	= bloques / 8 / OSADA_BLOCK_SIZE;

	baseTablaAsignacionesBitmap = offsetAsignaciones;
	limiteTablaAsignacionesBitmap = offsetDatos -1;

    printf(GRN "\t\t OFFSETS \n" RESET);
    printf("\t\t OffsetBitmap: %d bytes\n", offsetBitmap);
	printf("\t\t OffsetTablaArchivos: %d bytes\n", offsetTablaArchivos);
	printf("\t\t OffsetAsignaciones: %d bytes\n", offsetAsignaciones);
	printf("\t\t OffsetDatos: %d bytes\n", offsetDatos);
}

char borrarArchivo(char* path)
{
	//lee la tabla de archivos, pone en cero/borrado el estado del archivo y actualiza el bitmap
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	int posicion;

	osada_file* archivo = buscarArchivo(path, &posicion);
	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo\n" RESET);
		return 'n';
	}

    archivo->state = 0;
    escribirArchivo(posicion, archivo);

    //TODO: falta actualizar el bitmap

	printf(YEL "\t Se borro el archivo: %s\n" RESET, archivo->fname);
	return 's';
}

char borrarDirectorio(char* path)
{
	//lee la tabla de archivos, chequea que el directorio este vacio y pone en cero/borrado el estado del directorio
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	int posicion;

	osada_file* archivo = buscarArchivo(path, &posicion);
	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo\n" RESET);
		return 'n';
	}
	if (!esDirectorioVacio(posicion))
	{
		printf(RED "\t No se pudo borrar el directorio porque no esta vacio\n" RESET);
		return 'n';
	}
    archivo->state = 0;
    escribirArchivo(posicion, archivo);

	printf(YEL "\t Se borro el archivo: %s\n" RESET, archivo->fname);
	return 's';
}

osada_file* buscarArchivo(char* path, int* posicion)
{
	osada_file* archivo = malloc(sizeof(osada_file));
	memset(archivo, 0, sizeof(osada_file));

	int i;

    for(i=0; i< 2048; i++)
    {
    	leerArchivo(i, archivo);

    	if ((strcmp((char*)archivo->fname, nombre(path)) == 0) && (archivo->state != 0))//necesito saber el path entero para saber si tiene el mismo padre
    	{
    		if (archivo->parent_directory == padre(path))
    		{
				*posicion = i;
				return archivo;
    		}
    	}
    }
    //free(archivo);
    return archivo;
}

int buscarEspacioLibre()//busca el primer espacio libre en la tabla de archivos
{
	int i;
	osada_file* archivo = malloc(sizeof(osada_file));
	memset(archivo, 0, sizeof(osada_file));
	for (i = 0; i< 2048; i++)
	{
		leerArchivo(i, archivo);
		if(archivo->state == 0)
		{
			return i;
		}
	}
	return -1;
}

int buscarBitLibre(uint32_t* posicion) //busca el primer espacio libre en el bitmap en la region de la tabla de asignaciones
{
	uint32_t i;
	uint32_t contador = 0;
	bool valor;
	printf(YEL "\t En buscarBitLibre baseTablaAsignacionesBitmap: %d\n" RESET,baseTablaAsignacionesBitmap);
	printf(YEL "\t En buscarBitLibre limiteTablaAsignacionesBitmap: %d\n" RESET, limiteTablaAsignacionesBitmap);

	for (i = baseTablaAsignacionesBitmap; i<= limiteTablaAsignacionesBitmap; i++)
	{
		valor = bitarray_test_bit(bitVector, i);
		contador++;
		if(valor == Verdadero)
		{
			bitarray_set_bit(bitVector, i);
			*posicion = contador;
			return i;
		}
	}
	return -1;
}

char crearArchivo(char* path, int modo)
{
	int posicion;
	osada_file* archivo = buscarArchivo(path, &posicion);

	if (archivo == NULL)
	{
		if (agregarArchivo(path, modo) == -1)//si no hay espacio en la tabla de archivos
		{
			return 'n';
		}
		printf(GRN "\t Se creó el archivo: %s\n" RESET, nombre(path));
		return 's';
	}
	else if (archivo->parent_directory != padre(path))
	{
		if (agregarArchivo(path, modo) == -1)
		{
			return 'n';
		}
		printf(GRN "\t Se creó el archivo: %s\n" RESET, nombre(path));
		return 's';
	}
	printf(RED "\t Ya existe el archivo %s\n" RESET, nombre(path));
	return 'n';

}

void descargar()
{
	if (munmap(disco, tamanioArchivo) == -1)
	{
		printf(RED "Error en munmap.\n" RESET);
		return;
	}
	if(close(descriptorArchivo)<0)
	{
		printf(RED "Error en close.\n" RESET);
		return;
	}
}

void escribirAsignacion(uint32_t posicion, uint32_t* buf)
{
	memcpy(disco + offsetAsignaciones + (posicion * sizeof(uint32_t)), buf, sizeof(uint32_t));
}

void escribirBloque(uint32_t bloque, char* buf)
{
	memcpy(disco + offsetDatos + (bloque * OSADA_BLOCK_SIZE), buf, OSADA_BLOCK_SIZE);
}

void escribirArchivo(uint32_t posicion, osada_file* buf)
{
	memcpy(disco + offsetTablaArchivos + (posicion * sizeof(osada_file)), buf, sizeof(osada_file));
}

int esDirectorioVacio(int posicion)
{
	int i;
	osada_file* archivo = malloc(sizeof(osada_file));
	memset(archivo, 0, sizeof(osada_file));

    for(i=0; i< 2048; i++)
    {
    	leerArchivo(i, archivo);

    	if ((archivo->parent_directory == posicion) && (archivo->state != 0))
    	{
    		free(archivo);
    		return 0;
    	}
    }

    free(archivo);
	return 1;
}

int existeDirectorio(char* token, uint16_t* padre, int* posicion)
{
	osada_file archivo;
	memset(&archivo, 0, sizeof(osada_file));

	int i;

    for(i=0; i< 2048; i++)
    {
    	*posicion = i;
    	leerArchivo(i, &archivo);

    	if ((strcmp((char*)archivo.fname, token) == 0) && (archivo.state != 0))
    	{
    		if (archivo.parent_directory == *padre)
    		{
				*padre = i;
				return 1;
			}
    	}
    }

    return 0;
}

int existePath(char* path, int* pos)
{
	int existe = 1;
	uint16_t padre = 65535;
	//printf( "path %s\n", path);
	char *token = malloc(strlen(path)+1);
	char *pathRecibido = malloc(strlen(path)+1);
	memset(token, 0, strlen(path)+1);
	memset(pathRecibido, 0, strlen(path)+1);

	memcpy(pathRecibido, path, strlen(path) +1);
	token = strtok(pathRecibido, "/");

	while ((token != NULL) && (existe != 0))
	{
	//	printf(CYN "token: %s\n" RESET, token);
		existe = existeDirectorio(token, &padre, pos);
		token = strtok(NULL, "/");
	}
	//free(pathRecibido);
	if (existe == 0)
	{
		printf(YEL "\t No existe path: %s\n" RESET, path);
		return 0;
	}
	printf(CYN "\t Existe path: %s, posicion:%d\n" RESET, path, padre);
	return 1;
}

void* getAttr(char* path)
{
	osada_file archivo;
    int pos = 0;

    if (strcmp(path, "/") == 0)
    {
		t_stbuf* stbuf = malloc(sizeof(t_stbuf));
		stbuf->mode = S_IFDIR | 0755;
		stbuf->nlink = 2;
		stbuf->size = 0;
		return stbuf;
    }
	if (existePath(path, &pos))
	{
		leerArchivo(pos, &archivo);

		if (archivo.state != 0)
		{
			t_stbuf* stbuf = malloc(sizeof( t_stbuf));
			if (archivo.state == 2)//si es un directorio
			{
				stbuf->mode = S_IFDIR | 0755;
				stbuf->nlink = 2;
				stbuf->size = 0;
			}
			else
			{
				stbuf->mode = S_IFREG | 0444;
				stbuf->nlink = 1;
				stbuf->size = archivo.file_size;
			}

			return stbuf;
		}
	}
	return NULL;
}

int hayEspacioEnDisco(int cantidadBloques) //busco espacio para un cantidad de bloques que quiero escribir leyendo los bits de la tabla de asignaciones
{
	int contador = 0;
	off_t i;
	bool valor;
	printf(CYN "\t Chequeando si hay espacio en disco\n" RESET);
	printf(CYN "\t Necesito %d bloque/s\n" RESET, cantidadBloques);
//	printf(YEL "\t baseTablaAsignacionesBitmap: %d\n" RESET,baseTablaAsignacionesBitmap );
//	printf(YEL "\t limiteTablaAsignacionesBitmap: %d\n" RESET, limiteTablaAsignacionesBitmap);

	for (i = baseTablaAsignacionesBitmap; i<= limiteTablaAsignacionesBitmap; i++)
	{
		//bool bitarray_test_bit(t_bitarray *self, off_t bit_index)
	//	printf(YEL "\t Entre en hayEspacioEnDisco\n" RESET);
		valor = bitarray_test_bit(bitVector, i);//error aca, ver bien la base y el limite de la tabla de asignaciones
		if(valor == Verdadero)
		{
			contador++;
			if(contador == cantidadBloques)
			{
				printf(CYN "\t Hay espacio libre suficiente para escritura\n" RESET);
				return 1;
			}
		}
	}
	printf(YEL "\t Hay %d bloques libres para escritura\n" RESET, contador);
	return 0;
}

void inicializarDisco()
{
	mapearDisco("challenge.bin"); //mapearDisco("challenge.bin");
	leerHeader();
	levantarDatosGenerales(oheader);
	asignarOffsets();
	levantarBitmap();
	leerTablaArchivos();
}

void leerArchivo(uint32_t posicion, osada_file* buf)
{
	memcpy(buf, disco + offsetTablaArchivos + (posicion * sizeof(osada_file)), sizeof(osada_file));
}

void leerAsignacion(uint32_t posicion, uint32_t* buf)
{
	memcpy(buf, disco + offsetAsignaciones + (posicion * sizeof(uint32_t)), sizeof(uint32_t));
}

//void leerBloque(uint32_t cantidadBloques, char* buf)
//{
//	memcpy(buf, disco + (cantidadBloques * OSADA_BLOCK_SIZE), OSADA_BLOCK_SIZE);
//}

void leerDato(uint32_t posicion, osada_block* buf)
{
	memcpy(buf, disco + offsetDatos + (posicion * OSADA_BLOCK_SIZE), OSADA_BLOCK_SIZE);
	//memcpy(buf, disco + offsetDatos + (posicion * sizeof(osada_block)), sizeof(osada_block));
}

void leerHeader()
{
	memcpy(&oheader, disco, sizeof(osada_header));
}

void levantarBitmap()
{
//	int i;
//	//Bool valor;
	printf(GRN "\t\t Leyendo Bitmap.. \n" RESET);
	//char *bitmap = disco + OSADA_BLOCK_SIZE;

	bitVector = bitarray_create(disco, bitmapSize);
    maximoBit = bitarray_get_max_bit(bitVector);

	printf("\t\t BitMap size: %zu\n", bitVector->size);
	printf("\t\t Max Bit: %d\n", maximoBit );
	(maximoBit / 8 == bitmapSize) ? printf(GRN "\t\t Cantidad de bits correcta\n\n" RESET) : printf(RED "\t\t Cantidad de bits incorrecta\n\n" RESET);

/*
	for (i=0; i<= oheader.fs_blocks; i++)
	{
		if ((i % 8) == 0)
			printf("%s", "|" );

		valor = bitarray_test_bit(bitVector, i);
		switch(valor)
		{
			case Verdadero:
				printf(CYN "1" RESET);
				break;
			case Falso:
				printf(BLU "0" RESET);
				break;
		}
	}
	printf("\n\n");*/
}

void levantarDatosGenerales(osada_header oheader)
{

	bloques	= oheader.fs_blocks;// fileStat.st_size / OSADA_BLOCK_SIZE;
	bitmapSize	= bloques / 8 / OSADA_BLOCK_SIZE;
	int tamanioTablaAsig = bloques - 1025 - oheader.bitmap_blocks - oheader.data_blocks;
	dataBlocks	= oheader.data_blocks;

    printf(GRN "\t\t HEADER FILE SYSTEM OSADA \n" RESET);
    char id[6];
    strncpy(id, (char*)oheader.magic_number,6);
    printf("\t\t ID: %s \n", id);
    printf("\t\t Version: %d \n", oheader.version);
	printf("\t\t Tamanio de bloque: %d bytes\n", OSADA_BLOCK_SIZE);
	printf("\t\t Cantidad de bloques: %d bloques\n", bloques);
	printf("\t\t Tamanio de Bitmap: %d bloques (oheader.bitmap_blocks)\n", oheader.bitmap_blocks);//bitmapSize);
	printf("\t\t Tamanio de Bitmap: %d bloques (bitmapSize)\n", bitmapSize);
	printf("\t\t Tabla de archivos: %d bloques\n", FILETABLE);
	printf("\t\t Tabla de asignaciones: %d bloques\n", tamanioTablaAsig);
	printf("\t\t Cantidad de bloques de datos: %d bloques\n\n", dataBlocks);
}

//	offsetBitmap = OSADA_BLOCK_SIZE;
//	offsetTablaArchivos = OSADA_BLOCK_SIZE + (oheader.bitmap_blocks * OSADA_BLOCK_SIZE);
//	offsetAsignaciones = offsetTablaArchivos + (1024 * OSADA_BLOCK_SIZE);
//	offsetDatos = offsetAsignaciones + tamanioTablaAsig;

int mapearDisco(char* path)
{
    //struct stat sb;

	if((descriptorArchivo = open(path,O_RDWR)) == -1)
	{
		printf(RED "Error en open \n" RESET);
		return -1;
	}

    if(fstat(descriptorArchivo, &fileStat)== -1)
    {
    	printf(RED "Error en stat \n" RESET);
        return -1;
    }

    tamanioArchivo = fileStat.st_size;
	disco = mmap(NULL, tamanioArchivo, PROT_READ | PROT_WRITE, MAP_SHARED, descriptorArchivo, 0);

	if(disco == MAP_FAILED)
	{
		printf(RED "Error en mmap.\n" RESET);
		return -1;
	}

	posix_madvise(disco, tamanioArchivo, POSIX_MADV_SEQUENTIAL);
	return descriptorArchivo;
}

char* nombre(char* path)
{
	char* pathToken = malloc(strlen(path)+1);
	memset(pathToken, 0, strlen(path)+1);
	memcpy(pathToken, path, strlen(path) +1);

	char* token = malloc(strlen(path) +1);
	char* auxiliar = malloc(strlen(path) +1);

	memset(token, 0, strlen(path)+1);
	memset(auxiliar, 0, strlen(path)+1);

	token = strtok(pathToken, "/");

	while (token != NULL)
	{
		memcpy(auxiliar, token, strlen(token) +1);
		token = strtok(NULL, "/");
	}

	char* respuesta = malloc(strlen(auxiliar) +1);
	memset(respuesta, 0, strlen(auxiliar) +1);
	memcpy(respuesta, auxiliar, strlen(auxiliar) +1);

	return respuesta;
}

int padre(char* path)
{
	int posicion = 0;

	if((posicion = posicionUltimoToken(path)) == -1)
	{
		printf(RED "\t No se encontro el archivo padre\n" RESET);
		return -1;
	}

	if(posicion == 0)
	{
		return 65535;
	}

	char* pathPadre = malloc((sizeof(char) * posicion) + 1);
	memset(pathPadre, 0, (sizeof(char) * posicion) + 1);
	strncpy(pathPadre, path, posicion);

	osada_file* archivo = buscarArchivo(pathPadre, &posicion);

	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo padre\n" RESET);
		return -1;
	}

	return posicion;
}

int posicionUltimoToken(char* path)
{
	int i = 0;
	if(strlen(path) > 1)
	{
		for(i = strlen(path) - 2; i >= 0; i --)
		{
			if(path[i] == '/')
			{
				return i;
			}
		}
	}
	return -1;
}

/* para procesar pedido readdir(),
 * recibo un path de fuse y chequeo que exista
 * si existe armo una cadena con los nombres de todos los archivos y directorios contenidos en ese path */
void* readdir(char* path)
{
	osada_file archivo;
	char* buffer = NULL;
	int pos;
    int i;
    int existe = 1;
    int contadorArchivosEnPath = 0;

    if (strcmp(path, "/") == 0)
    {
    	pos = 65535;
    }
    else
    {
    	existe = existePath(path, &pos);
    }

	if (existe != 0) //si el path es valido busco cuantos archivos contiene para dimensionar la respuesta
	{
		for(i=0; i< 2048; i++)
		{
			leerArchivo(i, &archivo);

			if ((pos == archivo.parent_directory) && (archivo.state != 0))
			{
				contadorArchivosEnPath++;
			}
		}
	}
	else
	{
		//printf(YEL "El path no existe \n" RESET);
		return NULL;
	}

	printf("\t Cantidad de archivos en path: %d\n", contadorArchivosEnPath);

	if(contadorArchivosEnPath != 0)
	{
		buffer = malloc(contadorArchivosEnPath * ((sizeof(char) * OSADA_FILENAME_LENGTH) + 1));//le sumo 1 para agregar el caracter centinela despues de cada fname
		memset(buffer, 0, contadorArchivosEnPath * ((sizeof(char) * OSADA_FILENAME_LENGTH) + 1));

		for(i=0; i< 2048; i++) //armo la cadena que voy a enviar a fuse con los archivos y diectorios encontrados en el path
		{
			leerArchivo(i, &archivo);

			if ((pos == archivo.parent_directory) && (archivo.state != 0))
			{
				strcat(buffer, (char*) archivo.fname);
				//printf(MAG "\t Archivo para el path: %s\n" RESET, archivo.fname);
				strcat(buffer, "/");
			}
		}
		printf(CYN "\t Archivos en path: %s\n" RESET, buffer);
	}

	return buffer;
}

void* readFile(osada_file* archivo)
{
	int i;
	uint32_t offset = 0;
	uint32_t fileSize = archivo->file_size;
	void* respuesta = malloc(fileSize);
	memset(respuesta, 0, fileSize);

	int cantidadBloques = fileSize / OSADA_BLOCK_SIZE;

	if((fileSize % OSADA_BLOCK_SIZE) != 0)
	{
		cantidadBloques++;
	}
	printf(YEL "\t cantidadBloques: %d\n" RESET, cantidadBloques);

	uint32_t next_block = archivo->first_block;

	void *buffer = malloc(OSADA_BLOCK_SIZE * cantidadBloques);
	void* bufferAux = malloc(OSADA_BLOCK_SIZE);

	memset(buffer, 0, OSADA_BLOCK_SIZE * cantidadBloques);
	memset(bufferAux, 0, OSADA_BLOCK_SIZE);

	for (i=0; i < cantidadBloques; i++)
	{
		leerDato(next_block, bufferAux);
		memcpy(buffer + offset, bufferAux, OSADA_BLOCK_SIZE);
		offset += OSADA_BLOCK_SIZE;
		leerAsignacion(next_block, &next_block);
	}

	memcpy(respuesta, buffer, fileSize);

	return respuesta;
}

char renombrarArchivo(char* paths)
{
	//separa el path recibido en nuevo y viejo, lee la tabla de archivos y actualiza el nombre,
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	int posicion;
	//printf(YEL "\t En renombrar: Los paths concatenados son: %s\n" RESET, paths);
	char* viejo = malloc(strlen(paths)+1);
	char* nuevo = malloc(strlen(paths)+1);

	memset(viejo, 0, strlen(paths)+1);
	memset(nuevo, 0, strlen(paths)+1);

	viejo = strtok(paths, "*");
	nuevo = strtok(NULL, "*");

	osada_file* archivo = buscarArchivo(viejo, &posicion);
	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo: %s\n" RESET, viejo);
		return 'n';
	}
	if (strlen(nombre(nuevo)) > 17)
	{
		printf(RED "\t El nuevo nombre de archivo supera la cantidad maxima de caracteres (17): %s\n" RESET, nombre(viejo));
		return 'n';
	}
    strcpy((char*)archivo->fname,  nombre(nuevo));
    escribirArchivo(posicion, archivo);

	printf(GRN "\t Se cambio el nombre del archivo: %s por: %s\n" RESET, nombre(viejo), nombre(nuevo));
	return 's';
}

void writeFile(char* path, size_t size, void* bufWrite, int cantidadBloques, off_t offset)
{
/*  empezar a pedir bloques libres o los bloques que tiene asignados ese archivo en particular,
	llamar a escribirBloque hasta terminar */
	printf(MAG "\t Entre en writeFile\n" RESET);
	int i;
	int posicionArchivo;
	uint32_t* posicionBloque = malloc(sizeof(uint32_t));
	uint32_t* proximoBloque  = malloc(sizeof(uint32_t));
	printf(MAG "\t Necesito %d bloque/s para guardar un buf de size: %d\n" RESET, cantidadBloques, size);

	osada_file* archivo = buscarArchivo(path, &posicionArchivo);
	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);

	int bit = buscarBitLibre(posicionBloque);
    if(archivo->file_size == 0)//si es un archivo recien creado
    {
    	archivo->file_size = (uint32_t) size;
		archivo->first_block = *posicionBloque;
		//falta actualizar la fecha de ultima modif.
    	escribirArchivo(posicionArchivo, archivo);
    	printf(BLU "\t Por entrar al for\n" RESET);
		for(i = 0; i< (cantidadBloques -1); i++)
		{
			printf(YEL "\t i: %d\n" RESET,i);
			printf(GRN "\t El bit libre que me dieron es: %d\n" RESET, bit);

		  // bitarray_set_bit(bitVector, bit);
		   memcpy(bloque, bufWrite + (OSADA_BLOCK_SIZE * i), OSADA_BLOCK_SIZE);
		   escribirBloque(*posicionBloque, bloque);
		   bit = buscarBitLibre(proximoBloque);
		   escribirAsignacion(*posicionBloque, proximoBloque);
		   printf(YEL "\t Voy a ocupar el bloque: %d, y voy a linkear con el siguiente en: %d\n" RESET, *posicionBloque, *proximoBloque) ;
		   memcpy(posicionBloque,proximoBloque, sizeof(uint32_t));
		   //posicionBloque = proximoBloque;
		}
		printf(BLU "\t sali del for\n" RESET);
		int bytesResto = size % OSADA_BLOCK_SIZE;
		uint32_t* flag = malloc(sizeof(uint32_t));
		*flag = 65535;
		printf(GRN "\t Cacho final con fragmentacion interna: %d\n" RESET, bytesResto);
		memset(bloque, 0, OSADA_BLOCK_SIZE);
		memcpy(bloque, bufWrite + (OSADA_BLOCK_SIZE * i), bytesResto);

		escribirBloque(*posicionBloque, bloque);
		escribirAsignacion(*posicionBloque, flag);

	    printf(GRN "\t El bit que ocupe es: %d\n" RESET, bit);
		printf(GRN "\t Se guardo el archivo %s correctamente (mentira, todavia falta!!)\n" RESET, nombre(path));
    }
    else
    {
    	printf(RED "\t Falta codear cuando no es vacio\n" RESET);
    }
}

//funciones para probar la lectura correcta del disco----------------------------------------------------------------------
void leerTablaArchivos()
{
    osada_file archivo;
    int i;
    printf("tabla de archivos\n");
    printf("    Archivo.fname  parent_directory  file_size  state\n");
    for(i=0; i< 20; i++)
    {
       leerArchivo(i, &archivo);
       printf("%17s\t %8d\t %4d\t %4d\n\n", archivo.fname, archivo.parent_directory, archivo.file_size, archivo.state);
    }
}

void leerTablaAsignaciones()
{
	uint32_t asignacion;
    uint32_t i;
    printf("tabla de asignaciones\n");
    for(i=0; i< 100; i++)
    {
    	leerAsignacion(i, &asignacion);
        printf("%d \n", asignacion);
    }
}

void leerTablaDatos()
{
	osada_block bloque;
    int i;
    printf("tabla de datos\n");
    for(i=0; i< 100; i++)
    {
       leerDato(i, &bloque);
       printf("%s \n", bloque);
    }
}









