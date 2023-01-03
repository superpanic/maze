#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "tigr/tigr.h"

typedef struct cell {
	uint8_t row;
	uint8_t column;
	struct cell *north;
	struct cell *south;
	struct cell *east;
	struct cell *west;
	struct cell **links;
	int links_size;
	int links_count;
} Cell;

Cell **Grid;
int Rows;
int Columns;

void initialize(int rows, int columns);
void init_cell(Cell *c, uint8_t row, uint8_t column);
void configure_cells();
void link(Cell *ca, Cell *cb, bool bi);
bool unlink(Cell *ca, Cell *cb, bool bi);
bool linked(Cell *ca, Cell *cb);
Cell **links(Cell *c);
Cell **neighbors(Cell *c);

void binary_tree_maze();

bool find_link(Cell *ca, Cell *cb);
void free_all();
int index_at(int col, int row);
int row(int index);
int column(int index);
int size();
Cell *random_cell();

char *to_string();
void run_link_test();
void print_ascii_maze();

void die(char *e);

