/*
 ============================================================================
 Name        : mapa.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "mapa.h"

int main(int argc, char ** argv) {

	if (chequear_argumentos(argc, 3)) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
