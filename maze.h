#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include "tigr/tigr.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// A single cell in the maze, with links and neighbours.
typedef struct Cell {
	uint8_t column;
	uint8_t row;
	struct Cell *north;
	struct Cell *south;
	struct Cell *east;
	struct Cell *west;
	struct Cell **links;
	int links_size; // allocated in memory
	int links_count; // numnber of links currently connected to this cell
	bool solved; // true if distance is calculated for this cell
	int distance; // distance in steps from root
	bool path; // currently solved path
} Cell;

static const TPixel White = {255,255,255,255};
static const TPixel Black = {0,0,0,255};
static const TPixel Red = {255,0,0,255};
static const TPixel Green = {0,255,0,255};
static const TPixel Blue = {0,0,255,255};
static const TPixel Yellow = {255,255,0,255};
static const TPixel Gray = {220,220,220,255};

void initialize();
void init_cell(Cell *c, int columns, int row);
void configure_cells();
Cell *cell(int column, int row);
void link_cells(Cell *ca, Cell *cb, bool bi);
bool unlink_cells(Cell *ca, Cell *cb, bool bi);
bool linked(Cell *ca, Cell *cb);
bool find_link(Cell *ca, Cell *cb);
Cell **links(Cell *c);
Cell **neighbors(Cell *c, int *counter);
int neighbors_count(Cell *c);
Cell *get_random_neighbor(Cell *c);
Cell *get_random_neighbor_without_link(Cell *c);

void binary_tree_maze();
void sidewinder_maze();
void aldous_broder_maze();
void hunt_and_kill();
void wilson_maze();

bool array_includes_cell(Cell *arr[], Cell *c, int arr_len, int *index);
int remove_cell_from_array(Cell *arr[], int cell_index, int length);
Cell *calculate_distances(Cell *root);

int index_at(int col, int row);
int row(int index);
int column(int index);
int size();
Cell *random_cell_from_grid(int *index);
Cell *random_cell_from_array(Cell **array, int length, int *index);
void clear_maze_links();
clock_t performance_test(void (*alg)(), int runs);

size_t get_maze_string_size();
Cell **path_to(Cell *goal, int max_path);
void to_string(char str_out[], size_t str_size, bool print_distances);
void draw_start();
void draw_update(int slow);
void draw_end();
void draw(Cell **grid, Cell **breadcrumbs, int max_distance);
TPixel color_grid_distance(Cell *cell, int max);

void free_all();
void die(char *e, int n);
