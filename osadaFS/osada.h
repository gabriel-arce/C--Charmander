
#include <comunicacion.h>

#ifndef __OSADA_H__
#define __OSADA_H__

#define OSADA_BLOCK_SIZE 64
#define OSADA_FILENAME_LENGTH 17

#define FILETABLE 1024
#define MAX_FILES 2048
#define HEADER_BLOCK 1

typedef unsigned char osada_block[OSADA_BLOCK_SIZE];
typedef uint32_t osada_block_pointer;

// set __attribute__((packed)) for this whole section
// See http://stackoverflow.com/a/11772340/641451
#pragma pack(push, 1)

typedef enum{
	Falso,
	Verdadero,
} Bool;

typedef struct {
	unsigned char magic_number[7]; // OSADAFS
	uint8_t version;
	uint32_t fs_blocks; // total amount of blocks
	uint32_t bitmap_blocks; // bitmap size in blocks
	uint32_t allocations_table_offset; // allocations table's first block number
	uint32_t data_blocks; // amount of data blocks
	unsigned char padding[40]; // useless bytes just to complete the block size
} osada_header;

_Static_assert( sizeof(osada_header) == sizeof(osada_block), "osada_header size does not match osada_block size");

typedef enum __attribute__((packed)) {
    DELETED = '\0',
    REGULAR = '\1',
    DIRECTORY = '\2',
} osada_file_state;

_Static_assert( sizeof(osada_file_state) == 1, "osada_file_state is not a char type");

typedef struct {
	osada_file_state state;
	unsigned char fname[OSADA_FILENAME_LENGTH];
	uint16_t parent_directory;
	uint32_t file_size;
	uint32_t lastmod;
	osada_block_pointer first_block;
} osada_file;

typedef enum {
	SinUso,
	EnUso,
} osada_uso;

_Static_assert( sizeof(osada_file) == (sizeof(osada_block) / 2.0), "osada_file size does not half osada_block size");

struct NodoArchivo{
	char *nombre;						//Nombre del archivo
//	char *path;							//Nombre real
	int fd;								//descriptor de quien tiene prioridad
//	struct stat info;					//Estructura stat del archivo
	osada_uso enUso;					//Flag que determina si esta abie
	struct NodoArchivo *siguiente;	//Siguiente nodo
};

typedef struct {
	char *nombre;						//Nombre del archivo
	uint32_t fd;						//descriptor de quien tiene prioridad
	osada_uso enUso;					//Flag que determina si esta abie
} t_nodoArchivo;

//struct NodoArchivo *ListaArchivos;
struct NodoArchivo *ultimo;


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

//pthread_rwlock_t RWlock[2048];
pthread_rwlock_t lockTablaArchivos;
//pthread_rwlock_t mutexArchivo[2048];
//pthread_mutex_t mutexBitmap = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mutexFecha = PTHREAD_MUTEX_INITIALIZER;

//nuevas
int actualizarFirstBlockArchivo(osada_file* FCB);
void* attrRaiz();
void destruirSemaforos();
void devolverBitFirstBlockArchivo(uint32_t posicionArchivo, osada_file* FCB);
int existeArchivo(char* nombreArchivo, uint16_t parentDirectory, int posicion);
int intentarOAgregar(int parentDir, char* nombreArchivo, osada_file* nuevo);

//Funciones
char abrirArchivo(char* path);
//void actualizarFCBArchivo(int posicionArchivo, osada_file* FCB, uint32_t size);//, uint32_t* posicionBloque);

//int agregarArchivo(char* path, int modo, char* nombreArchivo);
//
//int agregarArchivo(char* path, int modo);
void agregarArchivoEnLista(osada_file archivo, t_list *lista);

void agregarBloques(uint32_t size, uint32_t newSize, uint32_t posicion);
void asignarOffsets();
char borrarArchivo(char* path);
char borrarDirectorio(char* path);
osada_file* buscarArchivo(char* nombre, int* posicion);
struct NodoArchivo *buscarArchivoEnLista(char *nombre);
int buscarEspacioLibre();//esta busca un espacio en la tabla de archivos
int buscarBitLibre(uint32_t* posicion);//esta la uso para buscar bloques libres en la tabla de datos
char buscarYtruncar(char* path, uint32_t newSize);
char cambiarUltimoAcceso(char* path);
int cantidadDeBloques(uint32_t size);
char crearArchivo(char* path, int modo);
void descargar();
void escribirArchivo(uint32_t posicion, osada_file* buf);
void escribirAsignacion(uint32_t posicion, uint32_t* buf);

void escribrirArchivoConOffset(uint32_t size, void* bufWrite, uint32_t offset, uint32_t* posicion);
void escribrirArchivoSinOffset(uint32_t size, void* bufWrite, uint32_t* posicion);
void escribirBloque(uint32_t bloque, char* buf);
int escribriBloquesEnteros(uint32_t size, void* bufWrite, uint32_t* posicion, int* desplazamiento);
//void escribirMenosQueUnBloqueAlFinal(void* bufWrite, uint32_t* posicion, int desplazamiento, int resto);
int escribirMenosQueUnBloqueAlPrincipio(uint32_t* size, void* bufWrite, uint32_t offset, uint32_t* posicion);
void escribirResto(int bytesResto, void* bufWrite, uint32_t* posicionBloque, int desplazamiento);
int esDirectorioVacio(int posicion);
int existePath(char* path, int* pos);
int existeDirectorio(char* token, uint16_t* padre, int* posicion);
char flushArchivo(char* path);
void* getAttr(char* path);
int chequearYReservarEspacioEnDisco(osada_file* FCB, uint32_t size, uint32_t offset,int posicionArchivo, int cantidadBloques);
void inicializarDisco();
void leerArchivo(uint32_t posicion, osada_file* buf);
void leerAsignacion(uint32_t posicion, uint32_t* buf);
void leerDato(uint32_t posicion, osada_block* buf);
void leerHeader();
t_list *leerTablaArchivos();
void leerTablaAsignaciones();
void leerTablaDatos();
void levantarDatosGenerales(osada_header oheader);
void levantarBitmap();
char liberarArchivo(char* path);
void liberarBits(uint32_t posicion);
void liberarRecursos();
int mapearDisco(char* path);
char* nombre(char* path);
time_t obtenerFecha();
int padre(char* path);
void posicionarme(uint32_t* offset, uint32_t* posicion);
int posicionUltimoToken(char* path);
void* readdir(char* path);
char renombrarArchivo(char* paths);
void* readFile(osada_file* archivo);
void* readBuffer(char* path, size_t* size, off_t* offset, uint32_t* tamanioBuffer);
void sacarBloques(uint32_t size, uint32_t newSize, uint32_t posicion);
char truncar(osada_file* FCB, uint32_t newSize, int posicion);
int writeBuffer(uint32_t* size, uint32_t* offset, char* path, void* bufWrite);
//void* writeFile(uint32_t* size, void* bufWrite, uint32_t offset, int posicionArchivo, osada_file* FCB);
void* writeFile(uint32_t* size, void* bufWrite, uint32_t offset, osada_file* FCB);
#pragma pack(pop)

#endif // __OSADA_H__


