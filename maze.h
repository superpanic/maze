#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "tigr/tigr.h"

typedef struct Cell {
	uint8_t row;
	uint8_t column;
	struct Cell *north;
	struct Cell *south;
	struct Cell *east;
	struct Cell *west;
	struct Cell **links;
	int links_size;
	int links_count;
} Cell;

Cell **Grid;
int Rows;
int Columns;

void initialize();
void init_cell(Cell *c, int columns, int row);
void configure_cells();
Cell *cell(int column, int row);
void link(Cell *ca, Cell *cb, bool bi);
bool unlink(Cell *ca, Cell *cb, bool bi);
bool linked(Cell *ca, Cell *cb);
bool find_link(Cell *ca, Cell *cb);
Cell **links(Cell *c);
Cell **neighbors(Cell *c);

void binary_tree_maze();
void sidewinder_maze();

void free_all();
int index_at(int col, int row);
int row(int index);
int column(int index);
int size();
Cell *random_cell();

size_t get_maze_string_size();
void to_string(char str_out[], size_t str_size);

void die(char *e);

