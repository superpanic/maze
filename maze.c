#include "maze.h"

#define COLS 2
#define ROWS 3
#define DEBUG true
#define LINKS_SIZE_STEP 4

int main(int argc, char *argv[]) {
	Columns = COLS;
	Rows = ROWS;

	if(argc >= 3) {
		int co = atoi(argv[1]);
		int ro = atoi(argv[2]);
		if(co>1 && ro>1) {
			Columns = co;
			Rows = ro;
		} else {
			die("Please provide parameters [columns]>1 and [rows]>1 ");
		}
	}
	
	void (*maze_algorithm)();
	maze_algorithm = &binary_tree_maze;

	if(argc > 3) {
		char *argument = argv[3];
		if(argument[0]=='-') {
			switch(argument[1]) {
				case 's':
					maze_algorithm = &sidewinder_maze;
					break;
			}
		}
	}

	initialize();
	(*maze_algorithm)();
	
	size_t str_size = get_maze_string_size();
	char maze_str[str_size];
	to_string(maze_str, str_size);
	printf("%s", maze_str);
	
	free_all();
	return 0;
}

void initialize() {
	int cell_count = Columns * Rows;
	Grid = (Cell**)malloc(cell_count * sizeof(Cell*));
	if(!Grid) die("Failed to allocate memory for grid cells!");
	for(int i=0; i<cell_count; i++) {
		Grid[i] = (Cell*) malloc(sizeof(Cell));
		if(!Grid[i]) die("Failed to allocate memory to cell.");
		init_cell(Grid[i], i % Columns, i / Columns);
		if(DEBUG) printf("cell %d: column: %d, row: %d\n", i, Grid[i]->column, Grid[i]->row);
	}
	configure_cells();
}

void init_cell(Cell *c, int column, int row) {
	c->column = column;
	c->row = row;
	c->links_size = LINKS_SIZE_STEP;
	c->links = (Cell **)calloc(c->links_size, sizeof(Cell*));
	if(!c->links) die("Failed to allocate memory to cell links array.");
	c->links_count = 0;
}

void configure_cells() {
	if(!Grid) die("Grid not initilized.");
	int cell_count = Columns * Rows;
	for(int i=0; i<cell_count; i++) {
		Cell *c = Grid[i];
		if(i>=Columns) c->north = Grid[i-Columns];
		if(row(i)<Rows-1) c->south = Grid[i+Columns];
		if(column(i)<Columns-1) c->east = Grid[i+1];
		if(column(i)>0) c->west = Grid[i-1];
	}
}

Cell *cell(int column, int row) {
	if(column < 0 || column >= Columns) return NULL;
	if(row < 0 || row >= Rows) return NULL;
	return Grid[index_at(column, row)];
}

void link(Cell *ca, Cell *cb, bool is_bidi) {
	if(ca->links_count >= ca->links_size) {
		ca->links_size = ca->links_size + LINKS_SIZE_STEP;
		ca->links = (Cell **)realloc(ca->links, ca->links_size * sizeof(Cell*));
		if(!ca->links) die("Failed to increase links array size!");
	}
	if(!linked(ca,cb)) {
		ca->links[ca->links_count] = cb;
		ca->links_count++;
	}
	if(is_bidi) link(cb, ca, false);
}

bool unlink(Cell *ca, Cell *cb, bool is_bidi) {
	bool is_found = false;
	for(int i=0; i<ca->links_count; i++) {
		if(!is_found)
			if(ca->links[i] == cb) is_found = true;
		if(is_found) 
			ca->links[i] = ca->links[i+1];
	}
	if(is_found) ca->links_count--;
	if(is_bidi) unlink(cb, ca, false);
	return is_found;
}

Cell** links(Cell *c) {
	return c->links;
}

bool linked(Cell *ca, Cell *cb) {
	return(find_link(ca,cb)>0);
}

bool find_link(Cell *ca, Cell *cb) {
	for(int i=0; i<ca->links_count; i++) {
		if(ca->links[i] == cb) return true;
	}
	return false;
}

Cell** neighbors(Cell *c) {
	if(DEBUG) 
		printf("    Cell **neighbors(Cell *c)\n    Warning: Remember to free() return value.\n");

	Cell **neighbors = (Cell**) calloc(4, sizeof(Cell*));
	int i=0;
	if(c->north) neighbors[i++] = c->north;
	if(c->south) neighbors[i++] = c->south;
	if(c->east) neighbors[i++] = c->east;
	if(c->west) neighbors[i++] = c->west;
	return neighbors;
}

//

void binary_tree_maze() {
	int cell_count = Columns * Rows;
	for(int i=0; i<cell_count; i++) {
		Cell *c = Grid[i];
		int j = 0;
		Cell *neighbors[2];
		for(int n=0;n<2;n++) neighbors[n] = NULL;
		if(c->north) neighbors[j++] = c->north;
		if(c->east) neighbors[j++] = c->east;
		if(j==0) continue;
		int rnd = rand() % j;
		Cell *neighbor = neighbors[rnd];
		link(c, neighbor, true);
	}
}

void sidewinder_maze() {
	for(int ro=0; ro<Rows; ro++) {
		Cell *corridor[Columns];
		int index=0;
		for(int co=0; co<Columns; co++) {
			Cell *c = cell(co, ro);
			corridor[index] = c;
			if(!c) continue;
			index++;
			bool at_eastern_boundary = (c->east == NULL);
			bool at_northern_boundary = (c->north == NULL);
			int rnd = rand() % 2; // random number between 0 and 1:
			bool should_close_out = at_eastern_boundary || (!at_northern_boundary && rnd==0);
			if(should_close_out) {
				int rnd_index = rand() % index;
				Cell *member = corridor[rnd_index];
				if(member->north) link(member, member->north, true);
				while(index > 0) {
					index--;
					corridor[index] = NULL;
				}
			} else {
				link(c, c->east, true);
			}
		}
	}
}

//

void free_all() {
	if(!Grid) return;
	int cell_count = Columns * Rows;
	for(int i=0; i<cell_count; i++) {
		if(Grid[i]->links) 
			free(Grid[i]->links);
		if(Grid[i]) 
			free(Grid[i]);
	}
	free(Grid);
}


int index_at(int col, int row) {
	return row * Columns + col;
}

int row(int index) {
	return index / Columns;
}

int column(int index) {
	return index % Columns;
}

Cell *random_cell() {
	int rnd = rand() % size();
	return Grid[rnd];
}

int size() {
	return Rows * Columns;
}

//

size_t get_maze_string_size() {
	size_t str_size = (Columns * 4 + 1) * (Rows * 2 + 1);
	str_size += Rows * 2; // for newlines, 2 newlines for each row
	str_size += 1;	      // for '\0'
	return str_size;
}

void to_string(char str_out[], size_t str_size) {

	str_out[str_size - 1] = '\0';

	char *str_header = str_out;

	strcpy(str_header, "+");
	str_header++;

	for (int col = 0; col < Columns; col++) {
		strcpy(str_header, "---+");
		str_header += 4;
	}

	strcpy(str_header, "\n");
	str_header++;

	for (int row = 0; row < Rows; row++) {
		int line_length = Columns * 4 + 1;
		char top[line_length + 1];    // +1 for '\0'
		char bottom[line_length + 1]; // +1 for '\0'

		top[line_length] = '\0';
		bottom[line_length] = '\0';

		char *top_header = top;
		strcpy(top_header, "|");
		top_header++;

		char *bottom_header = bottom;
		strcpy(bottom_header, "+");
		bottom_header++;

		for (int col = 0; col < Columns; col++) {
			Cell *c = Grid[index_at(col, row)];

			if (linked(c, c->east)) {
				strcpy(top_header, "    ");
			} else {
				strcpy(top_header, "   |");
			}
			top_header += 4;

			if (linked(c, c->south)) {
				strcpy(bottom_header, "   +");
			} else {
				strcpy(bottom_header, "---+");
			}
			bottom_header += 4;
		}

		strcpy(str_header, top);
		str_header += line_length;
		strcpy(str_header, "\n");
		str_header++;

		strcpy(str_header, bottom);
		str_header += line_length;
		strcpy(str_header, "\n");
		str_header++;
	}
}

//

void die(char *e) {
	printf("%s %s", e, "Exiting program.\n");
	free_all();
	exit(1);
}
