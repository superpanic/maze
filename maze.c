#include "maze.h"

#define COLS 4
#define ROWS 4
#define MAZE_DEBUG false
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
			die("Please provide parameters [columns]>1 and [rows]>1.");
		}
	} else {
		die("Please provide parameters [columns] and [rows].");
	}
	
	void (*maze_algorithm)();
	maze_algorithm = &binary_tree_maze;

	if(argc>3) {
		for(int i=argc-3; i<argc; i++) {
			char *argument = argv[i];
				if(argument[0]=='-') {
					switch(argument[1]) {
						case 'b':
							maze_algorithm = &binary_tree_maze;
							break;

						case 's':
							maze_algorithm = &sidewinder_maze;
							break;

						case 'd':
							Print_distances_flag = true;
							break;

						case 'w':
							Draw_maze_flag = true;
							break;

						default:
							die("   -s for sidewinder algorithm.\n   -d for distances\n");
							break;
				}
			}
		}
	}

	// create the maze
	initialize();
	(*maze_algorithm)();
	
	// solve the maze
	calculate_distances(Grid[0]);

	// print to terminal
	size_t str_size = get_maze_string_size();
	char maze_str[str_size];
	to_string(maze_str, str_size);
	printf("%s", maze_str);

	// draw to window
	if(Draw_maze_flag) draw();
	
	// free and exit
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
		if(MAZE_DEBUG) printf("cell %d: column: %d, row: %d\n", i, Grid[i]->column, Grid[i]->row);
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
	c->distance = 0;
	c->solved = false;
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

void calculate_distances(Cell *root) {
	int max_cells = 32;
	Cell *front[max_cells];
	front[0] = root;
	int front_count = 1; // counting root as #1
	root->solved = true;

	while(front_count>0){
		Cell *new_front[max_cells];
		int new_front_count = 0;

		for(int i=0; i<front_count; i++) {
			Cell *cell = front[i];
			for(int j=0; j<cell->links_count; j++) {
				Cell *linked = cell->links[j];
				if(linked->solved) continue; // if distance more than 0 skip
				linked->distance = cell->distance + 1;
				linked->solved = true;
				new_front[new_front_count] = linked;
				new_front_count++;
			}
		}

		front_count = new_front_count;
		for(int k=0; k<new_front_count; k++) front[k] = new_front[k];
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
	if(MAZE_DEBUG) 
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
				if(Print_distances_flag) sprintf(top_header, "%s%3d ", top_header, c->distance);
				else strcpy(top_header, "    ");
			} else {
				if(Print_distances_flag) sprintf(top_header, "%s%3d|", top_header, c->distance);
				else strcpy(top_header, "   |");
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

void draw() {
	int win_width = 320;
	int win_height = 240;
	
	Tigr* screen = tigrWindow(win_width, win_height, "Maze", 0);
	
	int cell_size = 10;
	
	int img_width = Columns * cell_size;
	int img_height = Rows * cell_size;
	
	int offx = (win_width-img_width)/2;
	int offy = (win_height-img_height)/2;

	int cell_count = Columns * Rows;
	
	while (!tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE)) {
		tigrClear(screen, White);
		for(int i=0; i<cell_count; i++) {
			Cell *c = Grid[i];
			int x1 = (column(i) * cell_size) + offx;
			int y1 = (row(i) * cell_size) + offy;
			int x2 = (column(i)+1) * cell_size + offx;
			int y2 = (row(i)+1) * cell_size + offy;
			if(!c->north) tigrLine(screen, x1,y1,x2,y1,Black); // north edge
			if(!c->west) tigrLine(screen, x1,y1,x1,y2,Black); // western edge
			if(!linked(c, c->east)) tigrLine(screen,x2,y1,x2,y2+1,Black);
			if(!linked(c, c->south)) tigrLine(screen,x1,y2,x2,y2,Black);
		}
		tigrUpdate(screen);
	}

	tigrFree(screen);
}

//

void die(char *e) {
	printf("%s %s", e, "Exiting program.\n");
	free_all();
	exit(1);
}
