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

char* disco;
off_t tamanioArchivo;
int32_t descriptorArchivo;
osada_header oheader;

int offsetBitmap;
int offsetTablaArchivos;
int offsetAsignaciones;
int offsetDatos;
int baseDatosBitmap;
int limiteDatosBitmap;

uint32_t  bitmapSize, dataBlocks, bloques, maximoBit;
t_bitarray* bitVector;
struct stat fileStat;

char abrirArchivo(char* path)
{
	struct NodoArchivo *archivoLista = (struct NodoArchivo *) malloc(sizeof(struct NodoArchivo));

	printf("Consultando archivo %s ..\n", path);

	if ((archivoLista = buscarArchivoEnLista(path)) == NULL){
		printf("Error al abrir archivo: archivo %s inexistente\n", path);
		return 'n';
	}

	else {
		if (archivoLista->enUso == EnUso){
			printf("Error al abrir archivo: archivo en uso\n");
			return 'n';
		}
	}

	//Si llego hasta aca, entonces puede abrir el archivo
	archivoLista->enUso = EnUso;

	//TODO: chequear que el archivo exista OK
	//TODO: ver de tener una tabla con archivos abiertos OK
	//TODO: ver el modo (lectura o escritura), KO
	return 's';

}

struct NodoArchivo *buscarArchivoEnLista(char *nombre){
	struct NodoArchivo *aux = ListaArchivos;

	while(aux != NULL && aux->nombre != nombre)
		aux = aux->siguiente;

	return aux;
}

void actualizarFCBArchivo(int posicionArchivo, osada_file* FCB, uint32_t size, uint32_t* posicionBloque)
{
	printf(MAG "\t entre en (FCB->file_size == 0)\n" RESET);

	buscarBitLibre(posicionBloque);
	FCB->file_size = (uint32_t) size;
	FCB->first_block = *posicionBloque;
	FCB->lastmod =(uint32_t) obtenerFecha();
	escribirArchivo(posicionArchivo, FCB);
	printf(CYN "\t actualice FCB->file_size: %d \n" RESET, FCB->file_size);
}

int agregarArchivo(char* path, int modo)
{
	int pos;
	char* nombreArchivo = nombre(path);

	if(strcmp(nombreArchivo, ".Trash-1000") == 0)
	{
		free(nombreArchivo);
		return -3;
	}
	if(strlen(nombreArchivo) > 17)
	{
	//	printf(RED "\t El nombre de archivo -" VIO "%s" RED "- supera el limite de caracteres maximo (17)\n" RESET, nombre(path));
		free(nombreArchivo);
		return -2;
	}
	if ((pos = buscarEspacioLibre()) == -1)// chequeo que exista espacio en la tabla de archivos, si hay agrego el pedido
	{
		//printf(RED "\t No hay espacio para crear el archivo %s\n" RESET, nombre(path));
		free(nombreArchivo);
		return -1;
	}

	int p = padre(path);
	if (p == -1)
	{
		//printf(RED "\t Salgo sin agregar el archivo %s\n" RESET, nombre(path));
		free(nombreArchivo);
		return -3;
	}

	osada_file* nuevo = malloc(sizeof(osada_file));
	memset(nuevo, 0, sizeof(osada_file));
	strcpy((char*)nuevo->fname, nombreArchivo);
	nuevo->file_size = 0;
	nuevo->lastmod =(uint32_t) obtenerFecha();
	nuevo->first_block = 65535;
	nuevo->state = modo;//si es archivo o directorio
	nuevo->parent_directory = p;
	escribirArchivo(pos, nuevo);
	free(nuevo);
	free(nombreArchivo);
	return 0;
}

void agregarBloques(uint32_t size, uint32_t newSize, uint32_t posicion)
{
	//printf(NAR "\t en agregarBloques() posicion es: %d\n" RESET, posicion);
	int i;
	uint32_t posicionAnterior;
	int cantidadBloquesActual = cantidadDeBloques(size);
	int cantidadBloquesExtra = cantidadDeBloques(newSize) - cantidadBloquesActual;

	printf(PINK "\t cantidadBloquesActual: %d \n" RESET, cantidadBloquesActual);
	printf(PINK "\t cantidadBloquesExtra: %d \n" RESET, cantidadBloquesExtra);

	while(posicion != -1)
	{
		posicionAnterior = posicion;
		leerAsignacion(posicion, &posicion);
	}

	for(i = 0; i< cantidadBloquesExtra; i++)
	{
		buscarBitLibre(&posicion);
		escribirAsignacion(posicionAnterior, &posicion);
		posicionAnterior = posicion;
	}

	posicion = -1;

	escribirAsignacion(posicionAnterior, &posicion);
	//printf(NAR "\t saliendo de agregarBloques() \n" RESET);
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

	baseDatosBitmap = offsetDatos / OSADA_BLOCK_SIZE;
	limiteDatosBitmap = bloques -1;

    printf(GRN "\t\t OFFSETS \n" RESET);
    printf("\t\t OffsetBitmap: %d bytes\n", offsetBitmap);
	printf("\t\t OffsetTablaArchivos: %d bytes\n", offsetTablaArchivos);
	printf("\t\t OffsetAsignaciones: %d bytes\n", offsetAsignaciones);
	printf("\t\t OffsetDatos: %d bytes\n", offsetDatos);

	printf("\t\t bloque base de bloques de datos Bitmap: %d \n", baseDatosBitmap);
	printf("\t\t bloque limite de bloques de datos en Bitmap: %d \n", limiteDatosBitmap);

}

char borrarArchivo(char* path)
{
	//lee la tabla de archivos, pone en cero/borrado el estado del archivo y actualiza el bitmap
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	int posicion;

	osada_file* FCB = buscarArchivo(path, &posicion);
	if (FCB == NULL)
	{
		printf(RED "\t No se encontro el archivo\n" RESET);
		return 'n';
	}

	FCB->state = 0;
	liberarBits(FCB->first_block);
	printf( "\t Se borro el archivo: %s\n", FCB->fname);
    escribirArchivo(posicion, FCB);
    free(FCB);
	return 's';
}

char borrarDirectorio(char* path)
{
	//lee la tabla de archivos, chequea que el directorio este vacio y pone en cero/borrado el estado del directorio
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	int posicion;

	osada_file* FCB = buscarArchivo(path, &posicion);
	if (FCB == NULL)
	{
		printf(RED "\t No se encontro el archivo\n" RESET);
		return 'n';
	}
	if (!esDirectorioVacio(posicion))
	{
		printf(RED "\t No se pudo borrar el directorio porque no esta vacio\n" RESET);
		free(FCB);
		return 'n';
	}
	FCB->state = 0;
    escribirArchivo(posicion, FCB);
	printf( "\t Se borro el archivo: %s\n", FCB->fname);
	free(FCB);
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
				return archivo;
    		}
    	}
    }

    free(nombreArchivo);
    free(archivo);
    return NULL;
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
			free(archivo);
			return i;
		}
	}

	free(archivo);
	return -1;
}

int buscarBitLibre(uint32_t* posicion) //busca el primer espacio libre en el bitmap en la region de los bloques de datos
{
	uint32_t i;
	uint32_t contador = 0;
//	printf(CYN "\t Entre en  buscarBitLibre\n" RESET);
//	printf(CYN "\t En buscarBitLibre limiteDatosBitmap: %d\n" RESET, limiteDatosBitmap);

	for (i = baseDatosBitmap; i<= limiteDatosBitmap; i++)
	{
		if(bitarray_test_bit(bitVector, i) == 0)//si un bit esta en 0 esta libre
		{
			bitarray_set_bit(bitVector, i);//pongo el bit en 1 (ocupado)
			*posicion = contador;
				//printf( "\t primer bit libre encontrado: %d (con respecto a la posicion cero del bitmap)\n" , i);
				//printf( "\t en posicion: %d (con respecto al offset que apunta al inicio de los bloques de datos)\n" , contador);
			return i;
		}
		contador++;
	}
	return -1;
}

char buscarYtruncar(char* path, uint32_t newSize)
{
	int posicion = -1;

	if (!existePath(path, &posicion))
	{
		printf(RED "\t En pedido truncate: No se encontro el path: " YEL "%s\n" RESET, path);
		return 'n';
	}

	osada_file* FCB = buscarArchivo(path, &posicion);
	//FCB->file_size = newSize;

	if (FCB == NULL)
	{
		char* nombreArchivo = nombre(path);
		printf(RED "\t En pedido truncate: No se encontro el archivo: " YEL "%s\n" RESET, nombreArchivo);
		free(nombreArchivo);
		return 'n';
	}

	return  truncar(FCB, newSize, posicion);
}

char cambiarUltimoAcceso(char* path)
{
	//lee la tabla de archivos y actualiza la fecha de ultima modificacion
	//devuelve 's' para indicar ok al cliente o 'n' si fallo el pedido
	int posicion;

	osada_file* FCB = buscarArchivo(path, &posicion);
	if (FCB == NULL)
	{
		printf(RED "\t No se encontro el archivo \n" RESET);
		return 'n';
	}

	FCB->lastmod = (uint32_t) obtenerFecha();
    escribirArchivo(posicion, FCB);
    free(FCB);
    printf("\t Fecha actualizada \n");
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
	int posicion;
	osada_file* archivo = buscarArchivo(path, &posicion);
	char* nombreArchivo = nombre(path);

	if ((archivo == NULL) || (archivo->parent_directory != padre(path)))// si el archivo no existe, o esta ubicado en una ruta distinta intento agregarlo a la tabla de archivos
	{
		if (archivo != NULL)
		{
			free(archivo);
		}

		int retorno = agregarArchivo(path, modo);
		switch(retorno)
		{
			case 0:
				printf( "\t Se creó el archivo: %s\n" , nombreArchivo);
				free(nombreArchivo);
				return 's';

			case  -1://si no hay espacio en la tabla de archivos
				printf( YEL "\t No hay espacio en la tabla de archivos \n" RESET);
				free(nombreArchivo);
				return 'n';

			case -2://nombre de archivo con mas de 17 caracteres
				printf( YEL "\t Nombre de archivo con mas de 17 caracteres \n" RESET);
				free(nombreArchivo);
				return 'x';

			case -3:
				//printf( RED "\t Error al crear archivo \n" RESET);
				free(nombreArchivo);
				return 'e';
		}
	}

    free(archivo);
	printf(RED "\t Ya existe el archivo %s\n" RESET, nombreArchivo);
	free(nombreArchivo);
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
		printf(RED "Error en close.\n" RESET);
		return;
	}
}

void escribirArchivo(uint32_t posicion, osada_file* buf)
{
	memcpy(disco + offsetTablaArchivos + (posicion * sizeof(osada_file)), buf, sizeof(osada_file));
}

int escribirArchivoAsignandoBloquesNuevos(void* bufWrite, int cantidadBloques, uint32_t* posicionBloque, uint32_t* proximoBloque)
{
	int i;
	int desplazamiento = 0;

	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);

	//printf(BLU "\t En escribir Archivo Asignando Bloques Nuevos()\n" RESET);

	for(i = 0; i< cantidadBloques; i++)//(cantidadBloques -1); i++)
	{
	   memcpy(bloque, bufWrite + desplazamiento, OSADA_BLOCK_SIZE);
	   desplazamiento += OSADA_BLOCK_SIZE;

	   escribirBloque(*posicionBloque, bloque);
	   buscarBitLibre(proximoBloque);
	   escribirAsignacion(*posicionBloque, proximoBloque);

	   //printf(PINK "\t Voy a ocupar el bloque: %d, y voy a linkear con el siguiente en: %d\n" RESET, *posicionBloque, *proximoBloque) ;
	   memcpy(posicionBloque, proximoBloque, sizeof(uint32_t));
	}

	//printf(CYN "\t sali\n" RESET);
	free(bloque);
	return desplazamiento;
}

void escribrirArchivoConOffset(uint32_t size, void* bufWrite, uint32_t offset, uint32_t* posicion)//posicion llega con el first block del archivo
{
	printf(FUC "\t En escribrirArchivoConOffset() el offset recibido es: %d \n" RESET, offset);

	posicionarme(&offset, posicion);//esta modifica el offset

	int desplazamiento = 0;

	if (offset > 0)//en este momento el offset esta entre 0 y OSADA_BLOCK_SIZE
	{
		desplazamiento = escribirMenosQueUnBloqueAlPrincipio(&size, bufWrite, offset, posicion);
	}

	int resto = escribriBloquesEnteros(size, bufWrite, posicion, &desplazamiento);

	if(resto >0)
	{
		 escribirMenosQueUnBloqueAlFinal( bufWrite, posicion, desplazamiento, resto);
	}
}

void escribrirArchivoSinOffset(uint32_t size, void* bufWrite, uint32_t* posicion)//posicion llega con el first block del archivo
{
	printf(FUC "\t En escribrirArchivoSinOffset() \n" RESET);

	int desplazamiento = 0;
	if (size > OSADA_BLOCK_SIZE)
	{
		int resto = escribriBloquesEnteros(size, bufWrite, posicion, &desplazamiento);

		if(resto > 0)
		{
			 escribirMenosQueUnBloqueAlFinal(bufWrite, posicion, desplazamiento, resto);
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
//	printf(PINK "\t resto: %d \n" RESET, resto);
	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);
	//printf(PINK "\t cantidadBloques: %d \n" RESET, cantidadBloques);

	for(i=0; i< cantidadBloques; i++)
	{
		//printf(NAR "\t i: %d \n" RESET, i);
		memcpy(bloque, bufWrite + *desplazamiento, OSADA_BLOCK_SIZE);
		*desplazamiento += OSADA_BLOCK_SIZE;
		escribirBloque(*posicion, bloque);
		leerAsignacion(*posicion, posicion);
		if(*posicion == -1)
		{
			//printf(PINK2 "\t saliendo de escribriBloquesEnteros() \n" RESET);
			return resto;
		}
	}

	free(bloque);
	//printf(PINK2 "\t saliendo de escribriBloquesEnteros() \n" RESET);
	return resto;
}

void escribirMenosQueUnBloqueAlFinal(void* bufWrite, uint32_t* posicion, int desplazamiento, int resto)
{
	printf(FUC "\t en escribirMenosQueUnBloqueAlFinal() \n" RESET);
	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);
	memcpy(bloque, bufWrite + desplazamiento, resto);
	escribirBloque(*posicion, bloque);
	//printf(MAG "\t despues de escribirBloque \n" RESET);
//	printf(PINK "\t saliendo de escribirMenosQueUnBloqueAlFinal() \n" RESET);
	free(bloque);
}

int escribirMenosQueUnBloqueAlPrincipio(uint32_t* size, void* bufWrite, uint32_t offset, uint32_t* posicion)
{
	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);
	int desplazamiento = OSADA_BLOCK_SIZE - offset;
	printf(PINK "\t offset recibido: %d \n" RESET, (uint32_t)offset);

	leerDato(*posicion, bloque);
	memcpy(bloque + offset, bufWrite, desplazamiento);
	escribirBloque(*posicion, bloque);
	size -=offset;
	leerAsignacion(*posicion, posicion);
//	printf(PINK "\t offset recibido: %d \n" RESET, (uint32_t)offset);

	free(bloque);
	return desplazamiento;
}

void escribirResto(int bytesResto, void* bufWrite, uint32_t* posicionBloque, int desplazamiento)
{
	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);

	//printf(PINK "\t bytesResto > 0: %d \n" RESET, bytesResto);

	uint32_t* flag = malloc(sizeof(uint32_t));
	*flag = -1;

	printf(FUC "\t Escribo %d bytes al final, fragmentacion interna en ultimo bloque: %d\n" RESET, bytesResto, OSADA_BLOCK_SIZE - bytesResto);

	memset(bloque, 0, OSADA_BLOCK_SIZE);
	memcpy(bloque, bufWrite + desplazamiento /*(OSADA_BLOCK_SIZE * i)*/, bytesResto);

	escribirBloque(*posicionBloque, bloque);
	escribirAsignacion(*posicionBloque, flag);

	//printf(FUC "\t termine de guardar todo \n" RESET);
	free(bloque);
	free(flag);
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
	//printf( GRN "path en existePath(): %s\n" RESET, path);
	char *token;
	char *pathRecibido = malloc(strlen(path)+1);
	memset(pathRecibido, 0, strlen(path)+1);

	memcpy(pathRecibido, path, strlen(path) +1);
	token = strtok(pathRecibido, "/");

	while ((token != NULL) && (existe != 0))
	{
	//	printf(CYN "token: %s\n" RESET, token);
		existe = existeDirectorio(token, &padre, pos);
		token = strtok(NULL, "/");
	}

	free(pathRecibido);

	if (existe == 0)
	{
		printf(YEL "\t No existe path: %s\n" RESET, path);
		return 0;
	}
	printf(CYN "\t Existe path: %s, posicion:%d\n" RESET, path, padre);

	return 1;
}

char flushArchivo(char* path)
{
	//TODO: ver si me sirve para mantener los archivos abiertos
	return 's';
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

int hayEspacioEnDisco(int cantidadBloques) //busco espacio para una cantidad de bloques que quiero escribir leyendo los bits del sector de bloques de datos
{
	int contador = 0;
	off_t i;
	//printf(CYN "\t Chequeando si hay espacio en disco\n" RESET);
	//printf(CYN "\t Necesito %d bloque/s\n" RESET, cantidadBloques);

	for (i = baseDatosBitmap; i<= limiteDatosBitmap; i++)
	{
		if(bitarray_test_bit(bitVector, i) == 0)
		{
			contador++;
			if(contador == cantidadBloques)
			{
				printf(FUC "\t Hay espacio libre suficiente para escritura\n" RESET);
				return 1;
			}
		}
	}

	printf(RED "\t Hay solamente %d bloque/s libre/s para escritura\n" RESET, contador);
	return 0;
}

void inicializarDisco()//TODO: dejar solo las llamadas utiles
{
	mapearDisco("basic.bin");
	leerHeader();
	levantarDatosGenerales(oheader);
	asignarOffsets();
	levantarBitmap();

	//TODO: Necesaria para generar la lista de archivos del FS en cuestion, eliminar de ser necesario el muestreo
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

void leerDato(uint32_t posicion, osada_block* buf)
{
	memcpy(buf, disco + offsetDatos + (posicion * OSADA_BLOCK_SIZE), OSADA_BLOCK_SIZE);
}

void leerHeader()
{
	memcpy(&oheader, disco, sizeof(osada_header));
}

void levantarBitmap()//TODO: probar esto y borrar lo que sobra
{
	printf(GRN "\t\t Leyendo Bitmap.. \n" RESET);

	bitVector = bitarray_create(disco + OSADA_BLOCK_SIZE, bitmapSize * OSADA_BLOCK_SIZE);
    maximoBit = bitarray_get_max_bit(bitVector);

	printf("\t\t BitMap size: %zu bytes\n", bitVector->size);
	printf("\t\t Max Bit: %d\n", maximoBit);
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
	//TODO: chequear que el archivo exista en la tabla de archivos abiertos, OK
	struct NodoArchivo *archivoLista;

	printf("Consultando archivo %s ..", path);

	if ((archivoLista = buscarArchivoEnLista(path)) == NULL){
		printf(RED "Error al abrir archivo.. archivo %s inexistente\n" RESET, path);
		return 'n';
	}

	//TODO: y marcarlo como cerrado OK
	archivoLista->enUso = SinUso;
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
	return respuesta;
}


time_t obtenerFecha()
{
 	time_t tiempo = time(0);

 	time(&tiempo);

 	time_t timeinfo =  (time_t)localtime(&tiempo);
 	//printf(YEL "\t Local Time: %s\r\n" RESET, asctime(timeinfo));
 	return timeinfo;

 	//Para cada uno de los campos..
 	// anio ---> timeinof->tm_year + 1900
 	//mes ---> timeinfo->tm_mon + 1
 	// dia ---> timeinfo->tm_mday
 	// y asi el resto..
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
		free(pathPadre);
		return -1;
	}

	free(archivo);
	free(pathPadre);
	return posicion;
}

void posicionarme(uint32_t* offset, uint32_t* posicion)
{
	int i;
	int nroBloque = 0;
	nroBloque = *offset / OSADA_BLOCK_SIZE;
	*offset = *offset % OSADA_BLOCK_SIZE;

	printf(PINK "\t en posicionarme() el nroBloque es: %d y el offset actualizado es: %d \n" RESET, nroBloque,(uint32_t) *offset);

	for(i = 0; i< nroBloque; i++)
	{
		leerAsignacion(*posicion, posicion);
	}
	printf(PINK "\t offset recibido: %d \n" RESET, (uint32_t) *offset);
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
	if(existePath(path, &posicion))
	{
		osada_file* archivo = buscarArchivo(path, &posicion);

		if (archivo == NULL)
		{
			char* nombreArchivo = nombre(path);
			printf(RED "\t No se encontro el archivo: %s\n" RESET, nombreArchivo);
			free(nombreArchivo);
			free(archivo);
			return NULL;
		}

		void* archivoCompleto = readFile(archivo);
		uint32_t bytes = archivo->file_size - (uint32_t)*offset;
		printf(BLU "\t El size del archivo completo es: %d bytes\n", archivo->file_size);

		if ((bytes <= *size) && (*offset == 0))
		{
			//printf(BLU "\t Los bytes en (bytes <= *size) son: %d bytes\n", bytes);
			memcpy(tamanioBuffer, &bytes, sizeof(uint32_t));
			free(archivo);
			return archivoCompleto;
		}
		else if (bytes <= *size)
		{
			//printf(BLU "\t Los bytes en (bytes <= *size)son: %d bytes y offset >0\n", bytes);
			void* respuesta = malloc(bytes);
			memset(respuesta, 0, bytes);
			memcpy(tamanioBuffer, &bytes, sizeof(uint32_t));
			memcpy(respuesta, archivoCompleto + *offset , bytes);
			free(archivoCompleto);
			free(archivo);
			return respuesta;
		}
	//	printf(CYN "\t Los bytes en (bytes > *size)son: %d bytes\n", bytes);
		void* respuesta = malloc(*size);
		memset(respuesta, 0, *size);
		memcpy(tamanioBuffer, size, sizeof(uint32_t));
		memcpy(respuesta, archivoCompleto + *offset, *size);
		free(archivo);
		free(archivoCompleto);
		return respuesta;
	}
	else
	{
		printf(RED "\t No encontré el path!\n" RESET);
		return NULL;
	}
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
	//printf(BLU "\t cantidadBloques: %d\n" RESET, cantidadBloques);

	uint32_t next_block = archivo->first_block;

	void* buffer = malloc(OSADA_BLOCK_SIZE * cantidadBloques);
	void* bufferAux = malloc(OSADA_BLOCK_SIZE);

	memset(buffer, 0, OSADA_BLOCK_SIZE * cantidadBloques);
	memset(bufferAux, 0, OSADA_BLOCK_SIZE);

	//printf("\t first_block: %d" , next_block);
	for (i=0; i < cantidadBloques; i++)
	{
		leerDato(next_block, bufferAux);
		memcpy(buffer + offset, bufferAux, OSADA_BLOCK_SIZE);
		offset += OSADA_BLOCK_SIZE;
		leerAsignacion(next_block, &next_block);
		//printf("\t %d" , next_block);
	}
	memcpy(respuesta, buffer, fileSize);

	free(buffer);
	free(bufferAux);

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

	osada_file* archivo = buscarArchivo(viejo, &posicion);
	if (archivo == NULL)
	{
		printf(RED "\t No se encontro el archivo: %s\n" RESET, viejo);
		free(nombreNuevo);
		free(nombreViejo);
		return 'n';
	}
	if (strlen(nombreNuevo) > 17)
	{

		printf(RED "\t El nuevo nombre de archivo supera la cantidad maxima de caracteres (17): %s\n" RESET, nombreViejo);
		free(archivo);
		free(nombreNuevo);
		free(nombreViejo);
		return 'n';
	}


    strcpy((char*)archivo->fname,  nombreNuevo);
    escribirArchivo(posicion, archivo);

	printf( "\t Se cambio el nombre del archivo: %s por: %s\n", nombreViejo, nombreNuevo);

	free(nombreNuevo);
	free(nombreViejo);
	free(archivo);
	return 's';
}

void sacarBloques(uint32_t size, uint32_t newSize, uint32_t posicion)
{
	printf(PINK "\t entrando a sacarBloques() \n" RESET);
	int i;
	uint32_t posicionAnterior;
	int cantidadBloquesNueva = cantidadDeBloques(newSize);
	printf(PINK "\t cantidadBloquesNueva: %d \n" RESET, cantidadBloquesNueva);

	for(i = 0; i< (cantidadBloquesNueva -1); i++)
	{
		leerAsignacion(posicion, &posicion);
	}

	leerAsignacion(posicion, &posicionAnterior);
	uint32_t flag = -1;
	escribirAsignacion(posicion, &flag);

	liberarBits(posicionAnterior);
	//printf(NAR "\t saliendo de sacarBloques() \n" RESET);
}

char truncar(osada_file* FCB, uint32_t newSize, int posicion)
{
	if(0 == newSize)
	{
		printf(NAR "\t no actualizo el file size, queda en %d bytes\n" RESET, FCB->file_size);
		free(FCB);
		return 's';
	}
	if(FCB->file_size > newSize)
	{
		sacarBloques(FCB->file_size, newSize, FCB->first_block);
		FCB->file_size = newSize;//si viene de write este newSize es igual a size + offset
	}
	else //(FCB->file_size < newSize)
	{
		agregarBloques(FCB->file_size, newSize, FCB->first_block);
		FCB->file_size = newSize;//si viene de write este newSize es igual a size + offset
	}

	printf(NAR "\t Actualizo el file size, queda en %d bytes\n" RESET, FCB->file_size);
	FCB->lastmod =(uint32_t) obtenerFecha();
	escribirArchivo(posicion, FCB);
//	printf(CYN "\t escribirArchivo\n" RESET);
	return 's';
}

int writeBuffer(uint32_t* size, uint32_t* offset, char* path, void* bufWrite)
{
	int posicion = -1;


	if(!existePath(path, &posicion))
	{
		printf(RED "\n\t No encontré el path!\n" RESET);
		return -1;
	}

	osada_file* FCBarchivo = buscarArchivo(path, &posicion);
	if (FCBarchivo == NULL)
	{
		char* nombreArchivo = nombre(path);
		printf(RED "\t En pedido write: No se encontro el archivo: " YEL "%s\n" RESET, nombreArchivo);
		free(nombreArchivo);
		return -1;
	}
	if (*size > 0)
	{
		uint32_t cantidadBloques = cantidadDeBloques(*size);

		if (hayEspacioEnDisco(cantidadBloques) == 0)
		{
			printf(RED "\n\t No hay espacio suficiente para escribir el archivo, cancelando operacion\n" RESET);
			free(FCBarchivo);
			return -2;
		}

		cantidadBloques = (uint32_t)(*size / OSADA_BLOCK_SIZE);
		writeFile(size, bufWrite, cantidadBloques, *offset, posicion, FCBarchivo);
	}

    free(FCBarchivo);
	return 0;
}

void* writeFile(uint32_t* size, void* bufWrite, int cantidadBloques, uint32_t offset, int posicionArchivo, osada_file* FCB)
{

	uint32_t* posicionBloque = malloc(sizeof(uint32_t));
	uint32_t* proximoBloque  = malloc(sizeof(uint32_t));

	if(cantidadBloques > 0)
	{
		printf(FUC "\t Necesito %d bloque/s para guardar un buf de: %d bytes\n" RESET, cantidadBloques, *size);
	}
	else
	{
		printf(FUC "\t Necesito menos que un bloque para guardar un buf de: %d bytes\n" RESET, *size);
	}

	void* bloque = malloc(OSADA_BLOCK_SIZE);
	memset(bloque, 0, OSADA_BLOCK_SIZE);

    if(FCB->file_size == 0)//si es un archivo recien creado
    {
    	//printf(FUC "\t es un archivo recien creado \n" RESET);

    	actualizarFCBArchivo(posicionArchivo, FCB, *size, posicionBloque);

    	int desplazamiento = escribirArchivoAsignandoBloquesNuevos(bufWrite, cantidadBloques, posicionBloque, proximoBloque);

    	int bytesResto = *size % OSADA_BLOCK_SIZE;

    	if(bytesResto > 0)
    	{
    		escribirResto(bytesResto, bufWrite, posicionBloque, desplazamiento);
    	}

		printf(FUC "\t Se guardo el archivo correctamente, size quedo en: %d bytes\n" RESET, *size);

		free(posicionBloque);
		free(proximoBloque);
		free(bloque);

		return size;
    }
	else
	{
		osada_file* archivo = malloc(sizeof(osada_file));
		memset(archivo, 0, sizeof(osada_file));
		memcpy(archivo, FCB, sizeof(osada_file));

		truncar(FCB, (*size + offset), posicionArchivo);//me aseguro que el archivo ya tenga asignados todos los bloques que necesita, si no los tiene se los agrego o se los saco si le sobran
		if (offset != 0)
		{
			escribrirArchivoConOffset(*size, bufWrite, offset, &(archivo->first_block));
		}
		else
		{
			escribrirArchivoSinOffset(*size, bufWrite, &(archivo->first_block));
		}

		printf(FUC "\t Se guardo el archivo correctamente \n" RESET);
		free(posicionBloque);
		free(proximoBloque);
		free(bloque);
		free(archivo);
		return size;
	}
}

//funciones para probar la lectura correcta del disco----------------------------------------------------------------------
void leerTablaArchivos()
{
		osada_file archivo;
		int i;
		printf(GRN "\t\t TABLA DE ARCHIVOS\n" RESET);
		printf("\t\t Archivo.fname  parent_directory  file_size  state\n");


		for(i = 0; i < 2048; i++){

			leerArchivo(i, &archivo);

			if (archivo.state != DELETED){
				//printf("%17s\t %8d\t %4d\t %4d\n\n", archivo.fname, archivo.parent_directory, archivo.file_size, archivo.state);

				if (archivo.state == REGULAR)
					printf("%25s\t %8d\t %4d\t Fichero\n", archivo.fname, archivo.parent_directory, archivo.file_size);
				if (archivo.state == DIRECTORY)
					printf("%25s\t %8d\t %4d\t Directorio\n", archivo.fname, archivo.parent_directory, archivo.file_size);

				agregarArchivoEnLista(archivo);

			}
		}

		mostrarLista();

}

void agregarArchivoEnLista(osada_file archivo){
	struct NodoArchivo *nuevo;
	struct NodoArchivo *aux;

	nuevo = (struct NodoArchivo *) malloc(sizeof(struct NodoArchivo));
	nuevo->nombre	= malloc(OSADA_FILENAME_LENGTH);

	memset(nuevo->nombre, '\0', sizeof(char) * OSADA_FILENAME_LENGTH);
	memcpy(nuevo->nombre, archivo.fname, strlen(archivo.fname));

	nuevo->fd			= descriptorArchivo;
	nuevo->enUso		= SinUso;
	nuevo->siguiente	= NULL;

	if(ListaArchivos == NULL){
		ListaArchivos = nuevo;

	} else {
		aux = ListaArchivos;
		while(aux->siguiente != NULL)
			aux = aux->siguiente;

		aux->siguiente = nuevo;
	}
}

void mostrarLista(){
//	struct t_listaArchivos *auxiliar;
	int i = 0;

//	auxiliar = primero;
//	ListaArchivos = primero;
	printf("\nMostrando la lista completa: \n");

	while(ListaArchivos != NULL){
		printf("Nombre: %s\n", ListaArchivos->nombre);
		printf("Descriptor: %d\n", ListaArchivos->fd);
		if (ListaArchivos->enUso == SinUso)
			printf("Sin Uso\n");
		else printf("En Uso\n");

		ListaArchivos = ListaArchivos->siguiente;
		i++;
	}

	if(i==0)
		printf("\nLa lista se encuentra vacia..\n");
}

void leerTablaAsignaciones()
{
	uint32_t asignacion;
		uint32_t i;
		printf(GRN "Tabla de asignaciones\n" RESET);
		for(i=0; i< 100; i++)
		{
			leerAsignacion(i, &asignacion);
				printf("\t La posicion: %d esta linkeada con:%d \n",i, asignacion);
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
