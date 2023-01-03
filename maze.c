#include "maze.h"

#define COLS 8
#define ROWS 5
#define DEBUG true
#define LINKS_SIZE_STEP 4

int main(int argc, char *argv[]) {
	Rows = ROWS;
	Columns = COLS;

	if(argc == 3) {
		int co = atoi(argv[1]);
		int ro = atoi(argv[2]);
		if(co>1 && ro>1) {
			Rows = ro;
			Columns = co;
		} else {
			die("Please provide parameters [columns]>1 and [rows]>1 ");
		}
	}

	initialize(Rows, Columns);
	binary_tree_maze();
	
	char *maze_str = to_string();
	printf("%s", maze_str);
	free(maze_str);
	
	free_all();
	return 0;
}

void initialize(int rows, int columns) {
	int cell_count = Columns * Rows;
	Grid = (Cell**)malloc(cell_count * sizeof(Cell*));
	if(!Grid) die("Failed to allocate memory for grid cells!");
	for(int i=0; i<cell_count; i++) {
		Grid[i] = (Cell*) malloc(sizeof(Cell));
		if(!Grid[i]) die("Failed to allocate memory to cell.");
		init_cell(Grid[i], i % Columns, i / Rows);
		if(DEBUG) printf("cell %d: column: %d, row: %d\n", i, Grid[i]->column, Grid[i]->row);
	}
	configure_cells();
}

void init_cell(Cell *c, uint8_t column, uint8_t row) {
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

void link(Cell *ca, Cell *cb, bool is_bidi) {
	if(ca->links_count >= ca->links_size) {
		ca->links_size = ca->links_size + LINKS_SIZE_STEP;
		ca->links = (Cell **)realloc(ca->links, ca->links_size * sizeof(Cell*));
		if(!ca->links) die("Failed to increase links array size!");
	}
	ca->links[ca->links_count] = cb;
	ca->links_count++;
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
	int cell_count = Rows * Columns;
	for(int i=0; i<cell_count; i++) {
		int j = 0;
		Cell *neighbors[2];
		for(int n=0;n<2;n++) neighbors[n] = NULL;
		if(Grid[i]->north) neighbors[j++] = Grid[i]->north;
		if(Grid[i]->east) neighbors[j++] = Grid[i]->east;
		if(j==0) continue;
		int rnd = rand() % j;
		Cell *neighbor = neighbors[rnd];
		link(Grid[i], neighbor, true);
	}
}

//

bool find_link(Cell *ca, Cell *cb) {
	for(int i=0; i<ca->links_count; i++) {
		if(ca->links[i] == cb) return true;
	}
	return false;
}

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
	return index / Rows;
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

char *to_string() {
	if(DEBUG) 
		printf("    char *toString()\n    Warning: Remember to free() return value.\n");

	int cell_count = Rows * Columns;
	char *str = (char*)malloc(cell_count+1);
	str[cell_count] = '\0';
	char *str_header = str;

	strcpy(str_header,"+");
	str_header++;
	
	for(int col=0; col<Columns; col++) {
		strcpy(str_header,"---+");
		str_header += 4;
	}

	strcpy(str_header, "\n");
	str_header++;

	for(int row=0; row<Rows; row++) {
		char *top = "|";
		char *bottom = "+";

		for(int col=0; col<Columns; col++) {
			Cell *c = Grid[index_at(col,row)];

			char *body = "   ";

			// top + body + east_boundary
			// |   |

			strcpy(str_header, top); // "|"
			str_header++;
			
			strcpy(str_header, body); // "   "
			str_header += 3;
			
			char *east_boundary = (linked(c, c->east)) ? " " : "|";
			strcpy(str_header, east_boundary);
			str_header++;

			// bottom + south_boundary + corner
			// +---+

			strcpy(str_header, bottom);
			str_header++;

			char *south_boundary = (linked(c, c->south)) ? "   " : "---";
			strcpy(str_header, south_boundary);
			str_header += 3;

			char *corner = "+";
			strcpy(str_header, corner);
			str_header++;

		}

		strcpy(str_header, top);
		str_header++;
		strcpy(str_header, "\n");
		str_header++;

		strcpy(str_header, bottom);
		str_header++;
		strcpy(str_header, "\n");
		str_header++;

	}

	return str;
}

void run_link_test() {
	int a_index = index_at(1,2);
	int b_index = index_at(2,2);
	link(Grid[a_index], Grid[b_index], true);
	int l = find_link(Grid[a_index], Grid[b_index]);
	printf("link found at %d\n", l);
}

void print_ascii_maze() {
	printf("\n");
	for(int y=0; y<Rows; y++) {
		for(int x=0; x<Columns; x++) {
			if(Grid[index_at(x,y)]->links_count>0)
				printf("[ ] ");
			else
				printf("[X] ");
		}
		printf("\n");
	}
}




//

void die(char *e) {
	printf("%s %s", e, "Exiting program.\n");
	free_all();
	exit(1);
}
