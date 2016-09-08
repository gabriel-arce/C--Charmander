/*
 * cliente-pokedex.h
 *
 *  Created on: 1/9/2016
 *      Author: utnso
 */

#ifndef CLIENTE_POKEDEX_H_
#define CLIENTE_POKEDEX_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>

#define TOTAL_ARGS 2

char * ip_pokedex;
int puerto_pokedex;
char * directorio_montaje;

int conectar_con_servidor_pkdx();

#endif /* CLIENTE_POKEDEX_H_ */
