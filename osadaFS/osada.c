#include "osada.h"

//colores para los prints en la consola
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define RESET "\x1B[0m"
#define NAR "\033[38;5;166m"//naranja
#define PINK "\033[38;5;168m"//rosa
#define FUC "\033[38;5;198m"//fucsia

#define VIO "\033[38;5;129m"//violeta
#define PINK2 "\033[38;5;207m"//violeta
#define YEL2 "\033[38;5;226m"//verde
#define RED2 "\033[38;5;196m"//ROJO
#define ORG "\033[38;5;214m"//naranja

char* disco;
off_t tamanioArchivo;
int32_t descriptorArchivo;
osada_header oheader;
struct NodoArchivo *ListaArchivos;
struct NodoArchivo *ultimo;

int offsetBitmap;
int offsetTablaArchivos;
int offsetAsignaciones;
int offsetDatos;
int baseDatosBitmap;
int limiteDatosBitmap;
int ultimoBit;

uint32_t  bitmapSize, dataBlocks, bloques;
t_bitarray* bitVector;
struct stat fileStat;

char abrirArchivo(char* path)
{

	return 's';
}

void devolverBitFirstBlockArchivo(uint32_t posicionArchivo, osada_file* FCB)
{
	bitarray_clean_bit(bitVector, baseDatosBitmap + FCB->first_block);
}

int actualizarFirstBlockArchivo(osada_file* FCB)
{
	uint32_t posicionBloque;
	buscarBitLibre(&posicionBloque);
	FCB->first_block = posicionBloque;
	//printf(CYN "\t FCB->first_block despues: %d, posicionArchivo: %d \n" RESET, FCB->first_block, posicionArchivo);
	return posicionBloque;
}

void agregarBloques(int cantidadBloquesActual, int cantidadBloquesNueva, uint32_t pos)
{
	int i;
	uint32_t posicionAnterior;
	uint32_t posicion = pos;

	int cantidadBloquesExtra = cantidadBloquesNueva - cantidadBloquesActual;

	//printf(FUC "\t bloques actuales: %d, se agregan: %d \n" RESET, cantidadBloquesActual, cantidadBloquesExtra);

	for(i = 1; i < cantidadBloquesActual; i++)
	{
		leerAsignacion(posicion, &posicion);//aca me tiro sigsegv
	}
	posicionAnterior = posicion;

	for(i = 0; i< cantidadBloquesExtra; i++)
	{
		buscarBitLibre(&posicion);
		escribirAsignacion(posicionAnterior, &posicion);
		posicionAnterior = posicion;
	}

	posicion = -1;
	escribirAsignacion(posicionAnterior, &posicion);
}

void asignarOffsets()
{
	int tamanioTablaAsig = (oheader.fs_blocks - 1025 - oheader.bitmap_blocks - oheader.data_blocks) * OSADA_BLOCK_SIZE;
	offsetBitmap = OSADA_BLOCK_SIZE;
	offsetTablaArchivos = OSADA_BLOCK_SIZE + (oheader.bitmap_blocks * OSADA_BLOCK_SIZE);
	offsetAsignaciones = offsetTablaArchivos + (1024 * OSADA_BLOCK_SIZE);
	offsetDatos = offsetAsignaciones + tamanioTablaAsig;

	bloques	= oheader.fs_blocks;
	bitmapSize	= bloques / 8 / OSADA_BLOCK_SIZE;

	baseDatosBitmap = offsetDatos / OSADA_BLOCK_SIZE;
	ultimoBit = baseDatosBitmap;
	limiteDatosBitmap = bloques -1;

    printf(GRN "\t\t OFFSETS \n" RESET);
    printf("\t\t OffsetBitmap: %d bytes\n", offsetBitmap);
	printf("\t\t OffsetTablaArchivos: %d bytes\n", offsetTablaArchivos);
	printf("\t\t OffsetAsignaciones: %d bytes\n", offsetAsignaciones);
	printf("\t\t OffsetDatos: %d bytes\n", offsetDatos);

	printf("\t\t bloque base de bloques de datos Bitmap: %d \n", baseDatosBitmap);
	printf("\t\t bloque limite de bloques de datos en Bitmap: %d \n", limiteDatosBitmap);

}

void* attrRaiz()
{
    //path == "/"
	t_stbuf* stbuf = malloc(sizeof(t_stbuf));
	stbuf->mode = S_IFDIR | 0777;
	stbuf->nlink = 2;
	stbuf->size = 0;
	stbuf->mtime = (uint32_t)obtenerFecha();
	return stbuf;
}

char borrarArchivo(char* path)
{
	int posicion;
	osada_file* FCB = buscarArchivo(path, &posicion);

	if (FCB == NULL)
	{
			printf(YEL "\t No se encontro el archivo\n" RESET);

		return 'n';
	}

	FCB->state = 0;
	escribirArchivo(posicion, FCB);
	liberarBits(FCB->first_block);

	printf( "\t Se borro el archivo: %s\n", FCB->fname);
	free(FCB);
	FCB = NULL;
	return 's';
}

char borrarDirectorio(char* path)
{
	int posicion;

	osada_file* FCB = buscarArchivo(path, &posicion);

	if (FCB == NULL)
	{
		printf(YEL "\t No se encontro el archivo\n" RESET);
		return 'n';
	}

	if (!esDirectorioVacio(posicion))
	{
		printf(YEL "\t No se pudo borrar el directorio porque no esta vacio\n" RESET);
		free(FCB);
		FCB = NULL;
		return 'n';
	}

	FCB->state = 0;
	escribirArchivo(posicion, FCB);

	printf( "\t Se borro el directorio: %s\n", FCB->fname);

	free(FCB);
	FCB = NULL;
	return 's';

}

osada_file* buscarArchivo(char* path, int* posicion)
{
	osada_file* archivo = malloc(sizeof(osada_file));
	memset(archivo, 0, sizeof(osada_file));
	char* nombreArchivo = nombre(path);
	int i;

    for(i=0; i< 2048; i++)
    {
    	leerArchivo(i, archivo);

    	if ((strcmp((char*)archivo->fname, nombreArchivo) == 0) && (archivo->state != 0))//necesito saber el path entero para saber si tiene el mismo padre
    	{
    		if (archivo->parent_directory == padre(path))
    		{
				*posicion = i;
				free(nombreArchivo);
				nombreArchivo = NULL;
				return archivo;
    		}
    	}
    }

    free(nombreArchivo);
    free(archivo);
    nombreArchivo = NULL;
    archivo = NULL;
    return NULL;
}

int buscarBitLibre(uint32_t* posicion)
{
	uint32_t i;
	uint32_t contador = 0;

//	for (i = baseDatosBitmap; i<= limiteDatosBitmap; i++)
//	{
//		if(bitarray_test_bit(bitVector, i) == 0)//si un bit esta en 0 esta libre
//		{
//			bitarray_set_bit(bitVector, i);//pongo el bit en 1 (ocupado)
//
//			*posicion = contador;
//			return i;
//		}
//		contador++;
//	}

	contador = ultimoBit - baseDatosBitmap;
	for (i = ultimoBit; i<= limiteDatosBitmap; i++)
	{
		if(bitarray_test_bit(bitVector, i) == 0)//si un bit esta en 0 esta libre
		{
			bitarray_set_bit(bitVector, i);//pongo el bit en 1 (ocupado)
			ultimoBit= i;
			*posicion = contador;
			return i;
		}
		contador++;
	}

	contador = 0;
	for (i = baseDatosBitmap; i< ultimoBit; i++)
	{
		if(bitarray_test_bit(bitVector, i) == 0)//si un bit esta en 0 esta libre
		{
			bitarray_set_bit(bitVector, i);//pongo el bit en 1 (ocupado)
			ultimoBit = i;
			*posicion = contador;
			return i;
		}
		contador++;
	}

	return -1;
}

char buscarYtruncar(char* path, uint32_t newSize)
{
	printf(YEL "\t Path: %s\n" RESET, path);

	int posicion = -1;

	osada_file* FCB = buscarArchivo(path, &posicion);

	if (FCB == NULL)
	{
		printf(RED "\t En pedido truncate: No se encontro el archivo\n" RESET);

		return 'n';
	}

//	if((FCB->first_block == 65535) && (newSize !=0))//si era un archivo recien creado le tengo que cambiar el first block
//	{
//		uint32_t posicionBloque;
//		buscarBitLibre(&posicionBloque);
//		FCB->first_block = posicionBloque;
//	}

	if(newSize == 0)
	{
		printf( NAR "\t new size: 0, no actualizo el file size \n " RESET);
		free(FCB);
		FCB = NULL;
		return 's';
	}

	uint32_t cantidadBloques = cantidadDeBloques(newSize);//esta cantidad es la que va a escribir en disco

	if (chequearYReservarEspacioEnDisco(FCB, newSize, 0, posicion, cantidadBloques) == 0) //este llama a truncar si hay espacio para reservarle o sacarle bloques
	{
		free(FCB);
		FCB = NULL;
		return 'x';// no hay bloques libres ERRNOSPC;
	}

	printf( NAR "\t truncate %d bytes\n " RESET, FCB->file_size);
	//char res = truncar(FCB, newSize, posicion);
	free(FCB);
	FCB = NULL;
	return 's';
}

char cambiarUltimoAcceso(char* path)
{
	int posicion;

	osada_file* FCB = buscarArchivo(path, &posicion);
	if (FCB == NULL)
	{
//		printf(RED "\t No se encontro el archivo \n" RESET);
		return 'n';
	}

	time_t timeinfo = obtenerFecha();
	FCB->lastmod = (uint32_t)timeinfo;
	escribirArchivo(posicion, FCB);

	free(FCB);
	FCB = NULL;
	printf("\t Fecha actualizada\n");

	return 's';
}

int cantidadDeBloques(uint32_t size)
{
	int cantidadBloques = size / OSADA_BLOCK_SIZE;
	if((size % OSADA_BLOCK_SIZE) != 0)
	{
		cantidadBloques++;
	}

	return cantidadBloques;
}

char crearArchivo(char* path, int modo)
{
	char* nombreArchivo = nombre(path);

//validaciones
	if(strcmp(nombreArchivo, ".Trash-1000") == 0)
	{
		printf( YEL "\t .Trash-1000 \n" RESET);
		free(nombreArchivo);
		nombreArchivo = NULL;
		return 't';
	}
	if(strlen(nombreArchivo) > 16)
	{
		printf( YEL "\t Nombre de archivo con mas de 16 caracteres \n" RESET);
		free(nombreArchivo);
		nombreArchivo = NULL;
		return 'x';
	}
	int p = padre(path);
	if (p == -1) //No encontre el directorio padre
	{
		free(nombreArchivo);
		nombreArchivo = NULL;
		return 't';
	}

//si es valido intento agregar a la tabla
	osada_file* nuevo = malloc(sizeof(osada_file));
	memset(nuevo, 0, sizeof(osada_file));
	strcpy((char*)nuevo->fname, nombreArchivo);
	nuevo->file_size = 0;
	nuevo->lastmod =(uint32_t) obtenerFecha();
	nuevo->first_block = 65535;
	nuevo->state = modo;//si es archivo o directorio
	nuevo->parent_directory = p;

	int retorno = intentarOAgregar(p, nombreArchivo, nuevo);
	if (retorno == 1)
	{
		//printf( "\t Se creó el archivo: %s\n" , nombreArchivo);
		free(nombreArchivo);
		nombreArchivo = NULL;
		free(nuevo);
		nuevo = NULL;
		return 's';
	}

	if(retorno == -1)//si no hay espacio en la tabla de archivos
	{
		printf( YEL "\t No hay espacio en la tabla de archivos \n" RESET);
		free(nombreArchivo);
		nombreArchivo = NULL;
		free(nuevo);
		nuevo = NULL;
		return 'n';
	}

	printf(RED "\t Ya existe el archivo %s\n" RESET, nombreArchivo);
	free(nombreArchivo);
	nombreArchivo = NULL;
	free(nuevo);
	nuevo = NULL;
	return 'e';
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
		return;
	}
}

void destruirSemaforos()
{
	pthread_rwlock_destroy(&lockTablaArchivos);
}

void escribirArchivo(uint32_t posicion, osada_file* buf)
{
	memcpy(disco + offsetTablaArchivos + (posicion * sizeof(osada_file)), buf, sizeof(osada_file));
}

void escribrirArchivoConOffset(uint32_t size, void* bufWrite, uint32_t offset, uint32_t* posicion)//posicion llega con el first block del archivo
{
	//printf(FUC "\t En escribrirArchivoConOffset() el offset recibido es: %d \n" RESET, offset);

	posicionarme(&offset, posicion);//esta modifica el offset

	int desplazamiento = 0;

	if (offset > 0)//en este momento el offset esta entre 0 y OSADA_BLOCK_SIZE
	{
		desplazamiento = escribirMenosQueUnBloqueAlPrincipio(&size, bufWrite, offset, posicion);
	}

	int resto = escribriBloquesEnteros(size, bufWrite, posicion, &desplazamiento);

	if(resto >0)
	{
		escribirResto(resto, bufWrite, posicion, desplazamiento);
	}
}

void escribrirArchivoSinOffset(uint32_t size, void* bufWrite, uint32_t* posicion)//posicion llega con el first block del archivo
{
	//printf(FUC "\t En escribrirArchivoSinOffset() \n" RESET);

	if (size > OSADA_BLOCK_SIZE)
	{
		int desplazamiento = 0;
		int resto = escribriBloquesEnteros(size, bufWrite, posicion, &desplazamiento);

		if(resto > 0)
		{
			escribirResto(resto, bufWrite, posicion, desplazamiento);
		}
	}
	else
	{
		escribirResto(size, bufWrite, posicion, 0);
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

int escribriBloquesEnteros(uint32_t size, void* bufWrite, uint32_t* posicion, int* desplazamiento)
{
	int i;
	int cantidadBloques = size / OSADA_BLOCK_SIZE;
	int resto = size % OSADA_BLOCK_SIZE;
	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);

	for(i=0; i< cantidadBloques; i++)
	{
		memcpy(bloque, bufWrite + *desplazamiento, OSADA_BLOCK_SIZE);
		*desplazamiento += OSADA_BLOCK_SIZE;
		escribirBloque(*posicion, bloque);
		leerAsignacion(*posicion, posicion);
		if(*posicion == -1)
		{
			free(bloque);
			bloque = NULL;
			return resto;
		}
	}

	free(bloque);
	bloque = NULL;
	return resto;
}

int escribirMenosQueUnBloqueAlPrincipio(uint32_t* size, void* bufWrite, uint32_t offset, uint32_t* posicion)
{
	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);
	int desplazamiento = OSADA_BLOCK_SIZE - offset;

	leerDato(*posicion, bloque);
	memcpy(bloque + offset, bufWrite, desplazamiento);
	escribirBloque(*posicion, bloque);
	size -=offset;
	leerAsignacion(*posicion, posicion);

	free(bloque);
	bloque = NULL;
	return desplazamiento;
}

void escribirResto(int bytesResto, void* bufWrite, uint32_t* posicionBloque, int desplazamiento)
{
	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);

	uint32_t* flag = malloc(sizeof(uint32_t));
	*flag = -1;

	printf(FUC "\t Se escriben %d bytes, fragmentacion interna: %d\n" RESET, bytesResto, OSADA_BLOCK_SIZE - bytesResto);

	memset(bloque, 0, OSADA_BLOCK_SIZE);
	memcpy(bloque, bufWrite + desplazamiento, bytesResto);

	escribirBloque(*posicionBloque, bloque);
	escribirAsignacion(*posicionBloque, flag);
	//printf(FUC "\t posicion ultimo bloque: %d\n" RESET, *posicionBloque);

	free(bloque);
	bloque = NULL;
	free(flag);
	flag = NULL;
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
    		archivo = NULL;
    		return 0;
    	}
    }

    free(archivo);
    archivo = NULL;
	return 1;
}

int existeArchivo(char* nombreArchivo, uint16_t parentDirectory, int posicion)
{
	osada_file* archivo = malloc(sizeof(osada_file));
	leerArchivo(posicion, archivo);

	if ((strcmp((char*)archivo->fname, nombreArchivo) == 0) && (archivo->state != 0))//necesito saber el path entero para saber si tiene el mismo padre
	{
		if (archivo->parent_directory == parentDirectory)
		{
			free(archivo);
			archivo = NULL;
			return 1;//todavia existe
		}
	}

	free(archivo);
	archivo = NULL;
	return 0;
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
	char *token;
	char *pathRecibido = malloc(strlen(path)+1);

	memset(pathRecibido, 0, strlen(path)+1);
	memcpy(pathRecibido, path, strlen(path) +1);
	token = strtok(pathRecibido, "/");

	while ((token != NULL) && (existe != 0))
	{
		existe = existeDirectorio(token, &padre, pos);
		token = strtok(NULL, "/");
	}

	free(pathRecibido);
	pathRecibido = NULL;
	if (existe == 0)
	{
		return 0;
	}
	printf(CYN "\t Existe path: %s, posicion:%d\n" RESET, path, padre);
	return 1;
}

char flushArchivo(char* path)
{
	//printf("Paso por flush..\n");
	return 's';
}

void* getAttr(char* path)
{
    int pos;
    int existe = existePath(path, &pos);
    if (!existe)
	{
		printf(YEL "\t No existe path: %s\n" RESET, path);
		return NULL;
	}
	//printf(CYN "\t Existe path: %s, posicion:%d\n" RESET, path, pos);

	osada_file archivo;
    leerArchivo(pos, &archivo);

	time_t tiempo =(time_t)archivo.lastmod;

	if (archivo.state != 0)
	{
		t_stbuf* stbuf = malloc(sizeof( t_stbuf));
		if (archivo.state == 2)//si es un directorio
		{
			stbuf->mode = S_IFDIR | 0777;//0755;
			stbuf->nlink = 2;
			stbuf->size = 0;
			stbuf->mtime = tiempo;

		}
		else
		{
			stbuf->mode = S_IFREG | 0777;//0444;
			stbuf->nlink = 1;
			stbuf->size = archivo.file_size;
			stbuf->mtime = tiempo;

		}

		return stbuf;
	}
	printf(YEL "\t No existe path: %s\n" RESET, path);
	return NULL;
}

int chequearYReservarEspacioEnDisco(osada_file* FCB, uint32_t size, uint32_t offset, int posicionArchivo, int cantidadBloques)
//busco espacio para una cantidad de bloques que quiero escribir y si me alcanza lo reservo
{
	int contador = 0;
	off_t i;
	int actualizado = -1;

	if(FCB->file_size == 0)//si era un archivo recien creado le tengo que cambiar el first block
	{
		actualizado = actualizarFirstBlockArchivo(FCB);
	}

//	for (i = baseDatosBitmap; i<= limiteDatosBitmap; i++)
//	{
//		if(bitarray_test_bit(bitVector, i) == 0)
//		{
//			contador++;
//			if(contador == cantidadBloques)
//			{
//				truncar(FCB, (size + offset), posicionArchivo);//me aseguro que el archivo ya tenga asignados todos los bloques que necesita, si no los tiene se los agrego o se los saco si le sobran
//				return 1;
//			}
//		}
//	}

	for (i = ultimoBit; i<= limiteDatosBitmap; i++)
	{
		if(bitarray_test_bit(bitVector, i) == 0)
		{
			contador++;
			if(contador == cantidadBloques)
			{
				truncar(FCB, (size + offset), posicionArchivo);//me aseguro que el archivo ya tenga asignados todos los bloques que necesita, si no los tiene se los agrego o se los saco si le sobran
				return 1;
			}
		}
	}
	for (i = baseDatosBitmap; i< ultimoBit; i++)
	{
		if(bitarray_test_bit(bitVector, i) == 0)
		{
			contador++;
			if(contador == cantidadBloques)
			{
				truncar(FCB, (size + offset), posicionArchivo);//me aseguro que el archivo ya tenga asignados todos los bloques que necesita, si no los tiene se los agrego o se los saco si le sobran
				return 1;
			}
		}
	}
	printf(YEL "\t No hay espacio suficiente en disco, necesito: %d bloque/s, hay %d bloque/s libre/s\n" RESET, cantidadBloques, contador);
	if (actualizado != -1) //si habia cambiado el first block pero no me alcanzaron los bits libres
	{
		devolverBitFirstBlockArchivo(posicionArchivo, FCB);
	}
	return 0;
}

void inicializarDisco()
{
	mapearDisco("pokedex.bin");
	leerHeader();
	levantarDatosGenerales(oheader);
	asignarOffsets();
	levantarBitmap();
}

void inicializarSemaforos()
{
	pthread_rwlock_init(&(lockTablaArchivos), NULL);
}

int intentarOAgregar(int parentDir, char* nombreArchivo, osada_file* nuevo)//busca el archivo en la tabla de archivos, si no existe, y puede, lo agrega
{
	int i;
	int posicion = -1;

	osada_file* archivo = malloc(sizeof(osada_file));
	memset(archivo, 0, sizeof(osada_file));

	for (i = 0; i< 2048; i++)
	{
		leerArchivo(i, archivo);

    	if ((strcmp((char*)archivo->fname, nombreArchivo) == 0) && (archivo->state != 0))
    	{
    		if (archivo->parent_directory == parentDir)
    		{
    			free(archivo);
    			archivo = NULL;

				return 0; //ya existe un archivo con el mismo nombre y ruta
    		}
    	}
    	else if(posicion == -1)
		{
    		if(archivo->state == 0)
    		{
    			posicion = i;
    		}
		}
	}

	if (posicion != -1)
	{
		escribirArchivo(posicion, nuevo);

		free(archivo);
		archivo = NULL;
		return 1;
	}

	free(archivo);
	archivo = NULL;

	return -1;
}

void leerArchivo(uint32_t posicion, osada_file* buf)
{
	memcpy(buf, disco + offsetTablaArchivos + (posicion * sizeof(osada_file)), sizeof(osada_file));
}

void leerAsignacion(uint32_t posicion, uint32_t* buf)
{
	memcpy(buf, disco + offsetAsignaciones + (posicion * sizeof(uint32_t)), sizeof(uint32_t));
}

void leerDato(uint32_t posicion, osada_block* buf)
{
	memcpy(buf, disco + offsetDatos + (posicion * OSADA_BLOCK_SIZE), OSADA_BLOCK_SIZE);
}

void leerHeader()
{
	memcpy(&oheader, disco, sizeof(osada_header));
}

void levantarBitmap()
{
	printf(GRN "\t\t Leyendo Bitmap.. \n" RESET);

	bitVector = bitarray_create(disco + OSADA_BLOCK_SIZE, bitmapSize * OSADA_BLOCK_SIZE);
	printf("\t\t BitMap size: %zu bytes\n", bitVector->size);

}

void levantarDatosGenerales(osada_header oheader)
{
	bloques	= oheader.fs_blocks;
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
	printf("\t\t Tamanio de Bitmap: %d bloques\n", oheader.bitmap_blocks);
	printf("\t\t Tabla de archivos: %d bloques\n", FILETABLE);
	printf("\t\t Tabla de asignaciones: %d bloques\n", tamanioTablaAsig);
	printf("\t\t Cantidad de bloques de datos: %d bloques\n\n", dataBlocks);
}

char liberarArchivo(char* path)
{
	return 's';
}

void liberarBits(uint32_t posicion)
{
	while(posicion != -1)//ver porque la ultima asignacion es un -1 en vez de un ffff
	{
		bitarray_clean_bit(bitVector, baseDatosBitmap + posicion);
		leerAsignacion(posicion, &posicion);
	}
}

void liberarRecursos()
{
	 bitarray_destroy(bitVector);
}

int mapearDisco(char* path)
{
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

	char* auxiliar = malloc(strlen(path) +1);
	memset(auxiliar, 0, strlen(path)+1);

	char* token;
	token = strtok(pathToken, "/");

	while (token != NULL)
	{
		memcpy(auxiliar, token, strlen(token) +1);
		token = strtok(NULL, "/");
	}

	char* respuesta = malloc(strlen(auxiliar) +1);
	memset(respuesta, 0, strlen(auxiliar) +1);
	memcpy(respuesta, auxiliar, strlen(auxiliar) +1);

	free(pathToken);
    free(auxiliar);
    pathToken = NULL;
    auxiliar = NULL;
	return respuesta;
}

time_t obtenerFecha()
{
 	time_t tiempo = time(0);
 	time(&tiempo);
 	return tiempo;

 }

int padre(char* path)
{
	int posicion = 0;

	if((posicion = posicionUltimoToken(path)) == -1)
	{
		//printf(RED "\t No se encontro el archivo padre\n" RESET);
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
		//printf(RED "\t No se encontro el archivo padre\n" RESET);
		free(pathPadre);
		pathPadre = NULL;
		return -1;
	}

	free(archivo);
	free(pathPadre);
	pathPadre = NULL;
	archivo = NULL;
	return posicion;
}

void posicionarme(uint32_t* offset, uint32_t* posicion)
{
	int i;
	int nroBloque = 0;
	nroBloque = *offset / OSADA_BLOCK_SIZE;
	*offset = *offset % OSADA_BLOCK_SIZE;

	//printf(PINK "\t en posicionarme() el nroBloque es: %d y el offset actualizado es: %d \n" RESET, nroBloque,(uint32_t) *offset);

	for(i = 0; i< nroBloque; i++)
	{
		leerAsignacion(*posicion, posicion);
	}
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

	if (existe == 0)
	{
		return NULL;
	}

	//si el path existe busco cuantos archivos contiene para dimensionar la respuesta
	for(i=0; i< 2048; i++)
	{
		leerArchivo(i, &archivo);

		if ((pos == archivo.parent_directory) && (archivo.state != 0))
		{
			contadorArchivosEnPath++;
		}
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
				strcat(buffer, "/");
			}
		}
		printf(CYN "\t Archivos en path: %s\n" RESET, buffer);
	}

	return buffer;
}

void* readBuffer(char* path, size_t* size, off_t* offset, uint32_t* tamanioBuffer)
{
	int posicion = -1;

	osada_file* archivo = buscarArchivo(path, &posicion);

	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo\n" RESET);
		return NULL;
	}
	void* archivoCompleto = readFile(archivo);

	uint32_t bytes = archivo->file_size - (uint32_t)*offset;
	printf(BLU "\t file_size: %d bytes\n", archivo->file_size);

	if ((bytes <= *size) && (*offset == 0))
	{
		memcpy(tamanioBuffer, &bytes, sizeof(uint32_t));
		free(archivo);
		archivo = NULL;
		return archivoCompleto;
	}
	else if (bytes <= *size)
	{
		void* respuesta = malloc(bytes);
		memset(respuesta, 0, bytes);
		memcpy(tamanioBuffer, &bytes, sizeof(uint32_t));
		memcpy(respuesta, archivoCompleto + *offset , bytes);
		free(archivoCompleto);
		free(archivo);
		archivo = NULL;
		archivoCompleto = NULL;
		return respuesta;
	}

	void* respuesta = malloc(*size);
	memset(respuesta, 0, *size);
	memcpy(tamanioBuffer, size, sizeof(uint32_t));
	memcpy(respuesta, archivoCompleto + *offset, *size);
	free(archivo);
	free(archivoCompleto);
	archivo = NULL;
	archivoCompleto = NULL;
	return respuesta;

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

	uint32_t next_block = archivo->first_block;

	void* buffer = malloc(OSADA_BLOCK_SIZE * cantidadBloques);
	void* bufferAux = malloc(OSADA_BLOCK_SIZE);

	memset(buffer, 0, OSADA_BLOCK_SIZE * cantidadBloques);
	memset(bufferAux, 0, OSADA_BLOCK_SIZE);

	//printf(CYN "\t %d-%d" RESET, next_block, bitarray_test_bit(bitVector, baseDatosBitmap + next_block));

	for (i=0; i < cantidadBloques; i++)
	{
		leerDato(next_block, bufferAux);
		memcpy(buffer + offset, bufferAux, OSADA_BLOCK_SIZE);
		offset += OSADA_BLOCK_SIZE;
		leerAsignacion(next_block, &next_block);
		//printf(BLU "\t %d-%d" RESET, next_block, bitarray_test_bit(bitVector, baseDatosBitmap + next_block));
	}

	memcpy(respuesta, buffer, fileSize);

	free(buffer);
	free(bufferAux);
	buffer = NULL;
	bufferAux = NULL;
	return respuesta;
}

char renombrarArchivo(char* paths)
{
	int posicion;
	char* viejo;
	char* nuevo;

	viejo = strtok(paths, "*");
	nuevo = strtok(NULL, "*");
	char* nombreViejo = nombre(viejo);
	char* nombreNuevo = nombre(nuevo);

	if (strlen(nombreNuevo) > 16)
	{
		printf(RED "\t El nuevo nombre de archivo supera la cantidad maxima de caracteres (16): %s\n" RESET, nombreViejo);
		free(nombreNuevo);
		free(nombreViejo);
		nombreNuevo = NULL;
		nombreViejo = NULL;
		return 'n';
	}

	osada_file* archivoNuevo = buscarArchivo(nuevo, &posicion);
	if (archivoNuevo != NULL)
	{
		printf(YEL "\t Ya existe el archivo: %s\n" RESET, nuevo);
		free(nombreNuevo);
		free(nombreViejo);
		nombreNuevo = NULL;
		nombreViejo = NULL;
		free(archivoNuevo);
		archivoNuevo = NULL;
		return 'x';
	}

	osada_file* archivo = buscarArchivo(viejo, &posicion);
	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo: %s\n" RESET, viejo);
		free(nombreNuevo);
		free(nombreViejo);
		nombreNuevo = NULL;
		nombreViejo = NULL;
		return 'x';
	}

	strcpy((char*)archivo->fname, nombreNuevo);
	escribirArchivo(posicion, archivo);

	printf( "\t Se cambio el nombre del archivo: %s por: %s\n", nombreViejo, nombreNuevo);

	free(nombreNuevo);
	free(nombreViejo);
	free(archivo);
	nombreNuevo = NULL;
	nombreViejo = NULL;
	archivo = NULL;
	return 's';
}

void sacarBloques( int cantidadBloquesNueva, uint32_t posicion)//esta posicion es el first block del archivo
{
	int i;
	uint32_t posicionAnterior;

	//printf(NAR "\t Empiezo a leer asignaciones de %d bloques del archivo\n", cantidadBloquesNueva);

	for(i = 1; i< cantidadBloquesNueva; i++)
	{
		leerAsignacion(posicion, &posicion);
		//printf("\t %d", posicion);
	}

	leerAsignacion(posicion, &posicionAnterior);
//	printf("\t %d" RESET, posicionAnterior);
	uint32_t flag = -1;
	escribirAsignacion(posicion, &flag);

	liberarBits(posicionAnterior);
}

char truncar(osada_file* FCB, uint32_t newSize, int posicionArchivo)
{
	if(newSize == 0)
	{
		//printf( "\t truncate %d bytes\n" , FCB->file_size);
		return 's';
	}

	int cantidadBloquesActual = cantidadDeBloques(FCB->file_size);
	int cantidadBloquesNueva = cantidadDeBloques(newSize);

	if(cantidadBloquesActual == cantidadBloquesNueva)
	{
		if(FCB->file_size != newSize)
		{
		printf( NAR"\t Agrego bytes pero no bloques\n");
		FCB->file_size = newSize;//si viene de write este newSize es igual a size + offset
		}
		return 's';
	}
	else if(cantidadBloquesActual > cantidadBloquesNueva)
	{
		//printf(NAR "\t llamo a sacarBloques() con: cantidadBloquesActual: %d, cantidadBloquesNueva: %d, FCB->first_block: %d\n", cantidadBloquesActual, cantidadBloquesNueva, FCB->first_block);
		printf( "\t cantidad bloques actual: %d, cantidad despues de truncar: %d, eliminando: %d bloques\n", cantidadBloquesActual, cantidadBloquesNueva, cantidadBloquesActual - cantidadBloquesNueva);
		sacarBloques(cantidadBloquesNueva, FCB->first_block);
		FCB->file_size = newSize;//si viene de write este newSize es igual a size + offset
	}
	else
	{
		printf( "\t cantidad bloques actual: %d, cantidad despues de truncar: %d, agregando: %d bloques\n", cantidadBloquesActual, cantidadBloquesNueva, cantidadBloquesNueva -cantidadBloquesActual );
		//printf(FUC "\t Agregando bloques, cantidadBloquesNueva: %d\n",  cantidadBloquesNueva);
		agregarBloques(cantidadBloquesActual, cantidadBloquesNueva, FCB->first_block);
		FCB->file_size = newSize;//si viene de write este newSize es igual a size + offset
	}

	FCB->lastmod =(uint32_t) obtenerFecha();
	escribirArchivo((uint32_t)posicionArchivo, FCB);

	//printf(  "\t truncate %d bytes\n " , FCB->file_size);
	return 's';
}

int writeBuffer(uint32_t* size, uint32_t* offset, char* path, void* bufWrite)
{
	int posicion = -1;

	osada_file* FCB = buscarArchivo(path, &posicion);
	if (FCB == NULL)
	{

		printf(RED "\t No se encontro el archivo \n" RESET);
		return -1;
	}

	uint32_t cantidadBloques = cantidadDeBloques(*size);//esta cantidad es la que va a escribir en disco

	if (chequearYReservarEspacioEnDisco(FCB, *size, *offset, posicion, cantidadBloques) == 0) //este llama a truncar si hay espacio para reservarle o sacarle bloques
	{
	//	printf(RED "\n\t No hay espacio suficiente para escribir el archivo\n" RESET);

		free(FCB);
		FCB = NULL;
		return -2;
	}

	printf( FUC "\t truncate %d bytes\n " RESET, FCB->file_size);
	//cantidadBloques = (uint32_t)(*size / OSADA_BLOCK_SIZE);//esta cantidad es la que va a escribir en disco
	writeFile(size, bufWrite, *offset, FCB);

    free(FCB);
    FCB = NULL;
	return 0;
}

void* writeFile(uint32_t* size, void* bufWrite, uint32_t offset, osada_file* FCB)
{
	if (offset != 0)
	{
		escribrirArchivoConOffset(*size, bufWrite, offset, &(FCB->first_block));
	}
	else
	{
		escribrirArchivoSinOffset(*size, bufWrite, &(FCB->first_block));
	}

	return size;
}


