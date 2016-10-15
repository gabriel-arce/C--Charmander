/*
 * osada.c
 *
 *  Created on: 30 ago. 2016
 *      Author: gabriel
 */

#include "testOsada.h"

int main(int argc, char **argv){

	// Verifica sin argumentos
	if (argc == 1){
		mostrarAyuda();
		return EXIT_SUCCESS;
		}

	// Verifica cantidad de argumentos
	if(argc != 2)
		mostrarMensajeDeError(CantidadArgumentosIncorrecta);

	// Abrir Archivos
//	if ((in = fopen(argv[1], "rb")) == NULL){
	if ((in = open(argv[1], O_RDWR, 0777)) == -1){
		mostrarMensajeDeError(NoSePudoAbrirIn);
		return EXIT_FAILURE;
	}

	// Stat del archivo
	if ((stat(argv[1], &fileStat)) < 0){
		mostrarMensajeDeError(ErrorStat);
		return EXIT_FAILURE;
	}

	//Mostrar info
	mostrarInfoFS();

	close(in);
	return EXIT_SUCCESS;

}


void mostrarAyuda(){
	puts("Ayuda");
}

void mostrarMensajeDeError(Error e){
	switch(e){
		case CantidadArgumentosIncorrecta:
			puts("Cantidad de argumentos incorrecta..");
			break;

		case NoSePudoAbrirIn:
			puts("No se pudo abrir archivo de entrada..");
			break;

		case ErrorEnLectura:
			puts("Error en lectura..");
			break;

		case ErrorEnEscritura:
			puts("Error en escritura..");
			break;

		case OtroError:
			puts("Error desconocido..");
			break;

		case ErrorStat:
			puts("Error al leer stat del archivo..");
			break;

		case ErrorMmap:
			printf("Error al mapear estructura..\n\n");
			break;

	}
}

//Mostrar titulo
void titulo(Titulo titulo){
	switch(titulo){
		case InformacionGeneral:
			printf("Informacion General\n");
			break;

		case 	Header:
			printf("Leyendo Header.. \n");
			break;

		case Bitmap:
			printf("Leyendo Bitmap.. \n");
			break;

		case TablaArchivos:
			printf("Leyendo Tabla de Archivos.. \n");
			break;

		case TablaAsignaciones:
			printf("Leyendo Tabla de Asignaciones.. \n");
			break;

		case BloqueDeDatos:
			printf("Leyendo Bloques de Datos.. \n");
			break;

		case InformacionFS:
			printf("Info de FS \n");
			break;

	}

	printf("------------------------------------\n");

}

//Mostrar info
void mostrarInfo(){

	titulo(InformacionGeneral);

	printf("File Size: \t\t\t%d bytes\n", fileStat.st_size);
	printf("Number of links: \t%d\n", fileStat.st_nlink);

	mostrarPermisos();
	mostrarInfoFS();

}

//Mostrar permisos
void mostrarPermisos(){
	printf("Permisos: \t\t\t");
	printf( (S_ISDIR(fileStat.st_mode)) ? DIRECTORIO : NADA);
	printf( (fileStat.st_mode & S_IRUSR) ? LECTURA: NADA);
	printf( (fileStat.st_mode & S_IWUSR) ? ESCRITURA: NADA);
	printf( (fileStat.st_mode & S_IXUSR) ? EJECUCION: NADA);
	printf( (fileStat.st_mode & S_IRGRP) ? LECTURA: NADA);
	printf( (fileStat.st_mode & S_IWGRP) ? ESCRITURA: NADA);
	printf( (fileStat.st_mode & S_IXGRP) ? EJECUCION: NADA);
	printf( (fileStat.st_mode & S_IROTH) ? LECTURA: NADA);
	printf( (fileStat.st_mode & S_IWOTH) ? ESCRITURA: NADA);
	printf( (fileStat.st_mode & S_IXOTH) ? EJECUCION: NADA);
	printf("\n\n");

	printf("El archivo %s un symbolic link\n\n", (S_ISLNK(fileStat.st_mode)) ? "es" : "no es" );
}

void mostrarInfoFS(){
	char *hdr = NULL;

	titulo(InformacionFS);

	//Intentar levantar todo con map..
	mapearArchivo();
	levantarDatosGenerales();
	levantarHeader();
	levantarBitmap();
	levantarTablaArchivos();
	levantarTablaAsignaciones();
	levantarBloquesDeDatos();

}

void mapearArchivo(){

	hdr = mmap((caddr_t)0, fileStat.st_size, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, in, 0);

	if(hdr == MAP_FAILED){
		perror("mmap header");
		mostrarMensajeDeError(ErrorEnLectura);
	}
}

void levantarDatosGenerales(){

	bloques			= fileStat.st_size / BLOCK_SIZE;
	bitmapSize	= bloques / 8 / BLOCK_SIZE;
	assignTable	= ((bloques - 1 - bitmapSize - 1024) * 4) / BLOCK_SIZE;
	dataBlocks	= bloques - 1 - bitmapSize - 1024 - assignTable;

	printf("Tamanio de bloque:\t\t%d bytes\n",	BLOCK_SIZE);
	printf("Cantidad de bloques:\t%d bloques\n", bloques);
	printf("Tamanio de Bitmap:\t\t%d bloques\n", bitmapSize);
	printf("Tabla de archivos:\t\t%d bloques\n", FILETABLE);
	printf("Tabla de asignaciones:\t%d bloques\n", assignTable);
	printf("Cantidad de bloques de datos:\t%d bloques\n\n", dataBlocks);



}

void 	levantarHeader(){
	titulo(Header);

	if(hdr != MAP_FAILED){

		memcpy(&osada_header, hdr, sizeof(osada_header));

		printf("Id: \t%s\n", osada_header.identificador);
		printf("Version: \t%d\n", osada_header.version);
		printf("Tamanio del FS: \t%d bloques\n", osada_header.bloquesFS);
		printf("Tamanio del Bitmap: \t%d bloques\n", osada_header.bloquesBitMap);
		printf("Inicio Tabla Asignaciones: \tbloque %d\n", osada_header.inicioTablaAsignacion);
		printf("Tamanio datos: \t%d bloques\n", osada_header.bloquesDatos);
		printf("Relleno: \t%s\n\n", osada_header.relleno);
		printf("Fin Header...\n\n");

	}
}

void levantarBitmap(){
	int i;
	Bool valor;

	if(hdr != MAP_FAILED){
		titulo(Bitmap);

		//char *bitmap = &hdr[BLOCK_SIZE]);
		char *bitmap = hdr + BLOCK_SIZE;

		t_bitarray *bitVector = bitarray_create(bitmap, bitmapSize);

		maximoBit	= bitarray_get_max_bit(bitVector);


		printf("BitMap size: %zu\n", bitVector->size);
		printf("Max Bit: %d\n", maximoBit );
		(maximoBit / 8 == bitmapSize) ? printf("Cantidad de bits correcta\n\n") : printf("Cantidad de bits incorrecta\n\n");

		for (i=0;i<= osada_header.bloquesFS;i++){
			if ((i % 8) == 0)
				printf("%s", "|" );

			valor = bitarray_test_bit(bitVector, i);
			switch(valor){
				case Verdadero: {
					printf("%d", 1);
					break;}
				case Falso: {
					printf("%d", 0);
					break;
				}
			}
		}
	}

}

void levantarTablaArchivos(){
	titulo(TablaArchivos);

	if( hdr != MAP_FAILED){

		char *filetable = hdr + (BLOCK_SIZE * (1 + bitmapSize));

		osada_file = malloc(sizeof(t_osada_file));

		//char *bitmap = &hdr[BLOCK_SIZE]);

		for(int i = 0; i < MAX_FILES; i++){
			memcpy(filetable + i, osada_file, sizeof(t_osada_file));
			//osada_filetable[i] = (t_osada_file *)(osada_file + i);
			printf("Estado: %s\n", osada_file->state);
			printf("Nombre del archivo: %s\n", osada_file->fname);
			printf("Bloque padre: %d\n", osada_file->parent_directory);
			printf("Tamanio del archivo: %d\n", osada_file->file_size);
			printf("Fecha ultima modificacion: %d\n", osada_file->lastmod);
			printf("Bloque inicial: %d\n", osada_file->first_block);
		}
	}

}

void levantarTablaAsignaciones(int in){
	if( hdr != MAP_FAILED){
		titulo(TablaAsignaciones);

		memcpy(osada_assignTable, (hdr + (osada_header->inicioTablaAsignacion )), BLOCK_SIZE * (sizeof (int)));
	}
}

void levantarBloquesDeDatos(){
	if( hdr != MAP_FAILED){
		titulo(BloqueDeDatos);


		memcpy(osada_blockData; (hdr + (osada_header->inicioTablaAsignacion ) + BLOCK_SIZE * (sizeof (int))), (osada_header->bloquesDatos)));
	}
}



