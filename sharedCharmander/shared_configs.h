/*
 * shared_configs.h
 *
 *  Created on: 11/8/2016
 *      Author: utnso
 */

#ifndef SHARED_CONFIGS_H_
#define SHARED_CONFIGS_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/config.h>
#include <error.h>

/*
 * DISCLAIMER:
 * En las funciones "get.." evalua la validez de la property
 * pasada por argumento. En caso de que no pertenezca dicha
 * funcion retornara -1 o NULL dependiendo la property.
 * Por lo cual se aconseja chequear el valor de retorno
 * antes de asignarla a la variable.
 */

/* returns 1 on success; -1 on error */
int chequear_argumentos(int argc, int shouldc);

void cargar_archivo_config(char ** ruta_config, void(*funcion_de_carga)(void**));

char* getStringProperty(t_config* config, char* property);
int getIntProperty(t_config* config, char* property);
long getLongProperty(t_config* config, char* property);
double getDoubleProperty(t_config* config, char* property);
t_list *getListProperty(t_config *config, char *property);

int string_count(char *text, char *pattern);				//Obtener cantidad de subcadenas

#endif /* SHARED_CONFIGS_H_ */
