#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "tigr/tigr.h"

// A single cell in the maze, with links and neighbours.
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
	bool solved;
	int distance;
} Cell;

Cell **Grid;

int Rows;
int Columns;

const TPixel White = {255,255,255,255};
const TPixel Black = {0,0,0,255};
const TPixel Red = {255,0,0,255};

bool Print_distances_flag = false;
bool Draw_maze_flag = false;
bool Print_path_flag = false;

void initialize();
void init_cell(Cell *c, int columns, int row);
void configure_cells();
Cell *calculate_distances(Cell *root);
Cell **path_to(Cell *goal, int max_path);
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
void to_string(char str_out[], size_t str_size, bool print_distances);
void draw(Cell **grid, Cell **breadcrumbs);

void die(char *e);

