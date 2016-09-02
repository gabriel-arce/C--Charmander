/*
 * shared_configs.c
 *
 *  Created on: 11/8/2016
 *      Author: utnso
 */

#include "shared_configs.h"

void cargar_archivo_config(char ** ruta_config, void (*funcion_de_carga)(void**)) {

	funcion_de_carga((void **) ruta_config);

}

int chequear_argumentos(int argc, int shouldc) {
	if (argc == 1) {
		error_show("__Debe ingresar %d argumentos en total\n", shouldc);
		return -1;
	}

	if (argc != shouldc) {
		error_show("__Incorrento el numero de argumentos ingresados\n");
		return -1;
	}

	return 1;
}

char* getStringProperty(t_config* config, char* property) {
	if (config_has_property(config, property))
		return string_duplicate(config_get_string_value(config, property));
	else
		return NULL;
}

int getIntProperty(t_config* config, char* property) {
	if (config_has_property(config, property))
		return config_get_int_value(config, property);
	return -1;
}

long getLongProperty(t_config* config, char* property) {
	if (config_has_property(config, property))
		return config_get_long_value(config, property);
	return -1;
}

double getDoubleProperty(t_config* config, char* property) {
	if (config_has_property(config, property))
		return config_get_double_value(config, property);
	return -1;
}

t_list *getListProperty(t_config *config, char *property) {
	if (config_has_property(config, property)) {
		char **items = config_get_array_value(config, property);
		t_list *ret = list_create();
		int cant = string_count(config_get_string_value(config, property), ",")	+ 1;

		int i;
		for (i = 0; i < cant; i++)
			list_add(ret, (void *) string_duplicate(items[i]));

		return ret;
	} else {
		return NULL;
	}
}

int string_count(char *text, char *pattern) {
	char **chunks = string_split(text, pattern);
	int ret;
	for (ret = 0; chunks[ret]; ret++)
		;
	return ret - 1;
}
