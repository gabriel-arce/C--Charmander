#include <tad_items.h>
#include <stdlib.h>
#include <curses.h>
#include <commons/collections/list.h>

#include "nivel.h"

/* NUMERO RANDOM */
void rnd(int *x, int max) {
	*x += (rand() % 3) - 1;
	*x = (*x < max) ? *x : max - 1;
	*x = (*x > 0) ? *x : 1;
}

int main(void) {
	t_list* items = list_create();
	int rows = 100, cols = 100;
	int q, p;
	int x = 1;
	int y = 1;
	int ex1 = 10, ey1 = 14;
	int ex2 = 20, ey2 = 3;

	/* Inicializa el espacio de dibujo */

	nivel_gui_inicializar();
	printf("Inicializando/n");

	nivel_gui_get_area_nivel(&rows, &cols);
	p = cols;
	q = rows;
	CrearPersonaje(items, '@', p, q);
	CrearPersonaje(items, '#', x, y);

	//Pokemons salvajes
	CrearEnemigo(items, '1', ex1, ey1);
	CrearEnemigo(items, '2', ex2, ey2);

	//PokeNest H, M, F
	//El ultimo parametro define la cantidad de instancias del pokemon que contiene
	CrearCaja(items, 'H', 26, 10, 5);
	CrearCaja(items, 'M', 8, 15, 3);
	CrearCaja(items, 'F', 19, 9, 2);

	/* Dibuja cada entidad en la lista de items */
	nivel_gui_dibujar(items, "Pueblo Paleta");



	while (1) {


		//Busco Random para mover a los enemigos a medida que nos movemos
		rnd(&ex1, cols);
		rnd(&ey1, rows);
		rnd(&ex2, cols);
		rnd(&ey2, rows);

		//Muevo todos los personajes, pasar coordenadas a mano.

		//Excepto estos que se mueven por random
		MoverPersonaje(items, '1', ex1, ey1);
		MoverPersonaje(items, '2', ex2, ey2);

		//Probar movimiento de personajes
		//MoverPersonaje(items, '@', p, q);
		MoverPersonaje(items, '@', 10, 10);

		//MoverPersonaje(items, '#', x, y);
		MoverPersonaje(items, '#', 30, 5);


		if (((p == 26) && (q == 10)) || ((x == 26) && (y == 10))) {
			restarRecurso(items, 'H');
		}
		if (((p == 19) && (q == 9)) || ((x == 19) && (y == 9))) {
			restarRecurso(items, 'F');
		}
		if (((p == 8) && (q == 15)) || ((x == 8) && (y == 15))) {
			restarRecurso(items, 'M');
		}


		//Volvemos a llamar para que pueda dibujar el mapa con los cambios hechos.
		nivel_gui_dibujar(items, "Pueblo Paleta");
	}

	//Limpio el mapa

	BorrarItem(items, '#');
	BorrarItem(items, '@');
	BorrarItem(items, '1');
	BorrarItem(items, '2');
	BorrarItem(items, 'H');
	BorrarItem(items, 'M');
	BorrarItem(items, 'F');
	nivel_gui_terminar();
}

