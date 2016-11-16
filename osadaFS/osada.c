#include "osada.h"

char abrirArchivo(char* path)
{
	//TODO: chequear que el archivo exista, ver de tener una tabla con archivos abiertos y el modo (lectura o escritura)
	return 's';
}

char flushArchivo(char* path)
{
	//TODO: ver si me sirve para mantener los archivos abiertos
	return 's';
}

char liberarArchivo(char* path)
{
	//TODO: chequear que el archivo exista en la tabla de archivos abiertos y marcarlo como cerrado
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
	nuevo->lastmod = 0;//TODO: ver de poner la fecha con time()?
	nuevo->first_block = 65535;
	nuevo->state = modo;//si es archivo o directorio
	nuevo->parent_directory = p;
	escribirArchivo(pos, nuevo);

	return 0;
}

void agregarBloques(uint32_t size, uint32_t newSize, uint32_t posicion)
{
	int i;
	uint32_t posicionAnterior;
	int cantidadBloquesActual = cantidadDeBloques(size);
	int cantidadBloquesExtra = cantidadDeBloques(newSize) - cantidadBloquesActual;

	while(posicion != 65535)
	{
		posicionAnterior = posicion;
		leerAsignacion(posicion, &posicion);
	}

	for(i = 0; i< (cantidadBloquesExtra -1); i++)
	{
		buscarBitLibre(&posicion);
		escribirAsignacion(posicionAnterior, &posicion);
		posicionAnterior = posicion;
	}

	posicion = 65535;
	escribirAsignacion(posicionAnterior, &posicion);
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

	osada_file* FCB = buscarArchivo(path, &posicion);
	if (FCB == NULL)
	{
		printf(RED "\t No se encontro el archivo\n" RESET);
		return 'n';
	}

	FCB->state = 0;
		escribirArchivo(posicion, FCB);

		liberarBits(FCB->first_block); //TODO: chequear que funcione el actualizar el bitmap

	printf(YEL "\t Se borro el archivo: %s\n" RESET, FCB->fname);
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
		return 'n';
	}
	FCB->state = 0;
		escribirArchivo(posicion, FCB);

	printf(YEL "\t Se borro el archivo: %s\n" RESET, FCB->fname);
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
		//free(path);
		//free(posicion);
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
	printf(CYN "\t En buscarBitLibre baseTablaAsignacionesBitmap: %d\n" RESET,baseTablaAsignacionesBitmap);
	printf(CYN "\t En buscarBitLibre limiteTablaAsignacionesBitmap: %d\n" RESET, limiteTablaAsignacionesBitmap);

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

int hayEspacioEnDisco(int cantidadBloques) //busco espacio para una cantidad de bloques que quiero escribir leyendo los bits de la tabla de asignaciones
{
	int contador = 0;
	off_t i;
	bool valor;
	//printf(CYN "\t Chequeando si hay espacio en disco\n" RESET);
	//printf(CYN "\t Necesito %d bloque/s\n" RESET, cantidadBloques);
//	printf(YEL "\t baseTablaAsignacionesBitmap: %d\n" RESET,baseTablaAsignacionesBitmap );
//	printf(YEL "\t limiteTablaAsignacionesBitmap: %d\n" RESET, limiteTablaAsignacionesBitmap);

	for (i = baseTablaAsignacionesBitmap; i<= limiteTablaAsignacionesBitmap; i++)
	{
		//bool bitarray_test_bit(t_bitarray *self, off_t bit_index)
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
	printf(RED "\t Hay solamente %d bloque/s libre/s para escritura\n" RESET, contador);
	return 0;
}

void inicializarDisco()//TODO: dejar solo las llamadas utiles
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
}

void leerHeader()
{
	memcpy(&oheader, disco, sizeof(osada_header));
}

void levantarBitmap()//TODO: probar esto y borrar lo que sobra
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

void liberarBits(uint32_t posicion)
{
	while(posicion != 65535)
	{
		leerAsignacion(posicion, &posicion);
		//poner en cero el bit del bitmap que corresponde a la posicion leida
	}
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
	//printf(BLU "\t cantidadBloques: %d\n" RESET, cantidadBloques);

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

void sacarBloques(uint32_t size, uint32_t newSize, uint32_t posicion)
{
	int i;
	uint32_t posicionAnterior;
	int cantidadBloquesNueva = cantidadDeBloques(newSize);

	for(i = 0; i< (cantidadBloquesNueva -1); i++)
	{
		leerAsignacion(posicion, &posicion);
	}

	leerAsignacion(posicion, &posicionAnterior);
	uint32_t flag = 65535;
	escribirAsignacion(posicion, &flag);

	liberarBits(posicionAnterior);
}

char truncar(char* path, uint32_t newSize)
{
	int posicion = -1;

	if (existePath(path, &posicion))
	{
		osada_file* FCB = buscarArchivo(path, &posicion);
		if (FCB == NULL)
		{
			printf(RED "\t En pedido truncate: No se encontro el archivo: " YEL "%s\n" RESET, nombre(path));
			return 'n';
		}
		if(FCB->file_size == newSize)
		{
			return 's';
		}
		else if(FCB->file_size > newSize)
		{
			sacarBloques(FCB->file_size, newSize, FCB->first_block);
		}
		else //(FCB->file_size < newSize)
		{
			agregarBloques(FCB->file_size, newSize, FCB->first_block);
		}
		FCB->file_size = newSize;
		//FCB->lastmod
		escribirArchivo(posicion, FCB);
		return 's';
	}
	else
	{
		printf(RED "\t En pedido truncate: No se encontro el path: " YEL "%s\n" RESET, path);
		return 'n';
	}
}

void writeFile(char* path, size_t size, void* bufWrite, int cantidadBloques, off_t offset)
{
/*  empezar a pedir bloques libres o los bloques que tiene asignados ese archivo en particular,
	llamar a escribirBloque hasta terminar */
	//printf(MAG "\t Entre en writeFile\n" RESET);
	int i;
	int posicionArchivo;
	uint32_t* posicionBloque = malloc(sizeof(uint32_t));
	uint32_t* proximoBloque  = malloc(sizeof(uint32_t));
	//printf(MAG "\t Necesito %d bloque/s para guardar un buf de size: %d\n" RESET, cantidadBloques, size);

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
			printf(BLU "\t i: %d\n" RESET,i);
			printf(BLU "\t El bit libre que me dieron es: %d\n" RESET, bit);

			// bitarray_set_bit(bitVector, bit);
			memcpy(bloque, bufWrite + (OSADA_BLOCK_SIZE * i), OSADA_BLOCK_SIZE);
			escribirBloque(*posicionBloque, bloque);
			bit = buscarBitLibre(proximoBloque);
			escribirAsignacion(*posicionBloque, proximoBloque);
			printf(BLU "\t Voy a ocupar el bloque: %d, y voy a linkear con el siguiente en: %d\n" RESET, *posicionBloque, *proximoBloque) ;
			memcpy(posicionBloque,proximoBloque, sizeof(uint32_t));
			//posicionBloque = proximoBloque;
		}
		printf(BLU "\t sali del for\n" RESET);
		int bytesResto = size % OSADA_BLOCK_SIZE;
		uint32_t* flag = malloc(sizeof(uint32_t));
		*flag = 65535;
		printf(BLU "\t Cacho util final: %d, fragmentacion interna en ultimo bloque: %d\n" RESET, bytesResto, 64 - bytesResto);
		memset(bloque, 0, OSADA_BLOCK_SIZE);
		memcpy(bloque, bufWrite + (OSADA_BLOCK_SIZE * i), bytesResto);

		escribirBloque(*posicionBloque, bloque);
		escribirAsignacion(*posicionBloque, flag);

			printf(BLU "\t El bit que ocupe es: %d\n" RESET, bit);
		printf(BLU "\t Se guardo el archivo " YEL "%s" BLU " correctamente " RED "(mentira, todavia falta)\n" RESET, nombre(path));
		}
		else
		{
			printf(RED "\t Falta codear cuando no es vacio\n" RESET);
		}
}

void* writeBuffer(size_t* size, off_t* offset, char* path, void* bufWrite)
{
	int posicion = -1;

	if(existePath(path, &posicion))
	{
		osada_file* FCBarchivo = buscarArchivo(path, &posicion);
		if (FCBarchivo == NULL)
		{
			printf(RED "\t En pedido write: No se encontro el archivo: " YEL "%s\n" RESET, nombre(path));
			return NULL;
		}

		int cantidadBloques = cantidadDeBloques((uint32_t)*size);

		if (hayEspacioEnDisco(cantidadBloques) != 0)
		{
			//printf(RED "\t Escribiendo en archivo un buffer de size: %d\n" RESET, *size);

			if(FCBarchivo->file_size == 0)
			{
				printf(BLU "\t Era un archivo con 0 bloques asignados, va a pasar a ocupar %d bloque/s nuevo/s\n" RESET, cantidadBloques);
				writeFile(path, *size, bufWrite, cantidadBloques, *offset);
				return size;
			}
			else
			{
				printf(BLU "\t El size del archivo antes de escribir es: %d bytes\n", FCBarchivo->file_size);
				printf(RED "\t Era un archivo con %d bloques asignados, va a pasar a ocupar %d bloque/s\n" RESET, cantidadDeBloques(FCBarchivo->file_size), cantidadBloques);
			//	void* archivo = readFile(FCBarchivo);
				//uint32_t bytes = archivo->file_size - (uint32_t)*offset;


//				if ((bytes <= *size) && (*offset == 0))
//				{
//					printf(GRN "\t Los bytes en (bytes <= *size) son: %d bytes\n", bytes);
//					//memcpy(tamanioBuffer,&(archivo->file_size), sizeof(uint32_t));
//					memcpy(tamanioBuffer, &bytes, sizeof(uint32_t));
//					return archivo;
//				}
//				else if (bytes <= *size)
//				{
//					printf(RED "\t Los bytes en (bytes <= *size)son: %d bytes y offset >0\n", bytes);
//					void* respuesta = malloc(bytes);
//					memset(respuesta, 0, bytes);
//					//*tamanioBuffer = (uint32_t)*size;
//					memcpy(tamanioBuffer, &bytes, sizeof(uint32_t));
//					memcpy(respuesta, archivo + *offset , bytes);
//					return respuesta;
//				}
//				printf(YEL "\t Los bytes en (bytes > *size)son: %d bytes\n", bytes);
//				void* respuesta = malloc(*size);
//				memset(respuesta, 0, *size);
//				//*tamanioBuffer = (uint32_t)*size;
//				memcpy(tamanioBuffer, size, sizeof(uint32_t));
//				memcpy(respuesta, archivo + *offset , *size);
//				return respuesta;
			}
		}
		else
		{
			printf(RED "\n\t No hay espacio suficiente para escribir el archivo, cancelando operacion\n" RESET);
		}
	}
	else
	{
		printf(RED "\n\t No encontré el path!\n" RESET);
		return NULL;
	}

	printf(RED "\t NO DEBERIA ENTRAR ACA\n" RESET);
	return NULL;
}

//funciones para probar la lectura correcta del disco----------------------------------------------------------------------
void leerTablaArchivos()
{
		osada_file archivo;
		int i;
		printf(GRN "\t\t TABLA DE ARCHIVOS\n" RESET);
		printf("\t\t Archivo.fname  parent_directory  file_size  state\n");


		for(i=0; i< 2048; i++){
			leerArchivo(i, &archivo);

			if (archivo.state != DELETED){
				//printf("%17s\t %8d\t %4d\t %4d\n\n", archivo.fname, archivo.parent_directory, archivo.file_size, archivo.state);

				if (archivo.state == REGULAR)
					printf("%25s\t %8d\t %4d\t Fichero\n", archivo.fname, archivo.parent_directory, archivo.file_size);
				if (archivo.state == DIRECTORY)
					printf("%25s\t %8d\t %4d\t Directorio\n", archivo.fname, archivo.parent_directory, archivo.file_size);
			}
		}

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
