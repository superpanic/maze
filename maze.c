#include "maze.h"

#define COLS 4
#define ROWS 4
#define MAZE_DEBUG false
#define LINKS_SIZE_STEP 4
#define MAZE_TIGR

int main(int argc, char *argv[]) {
	Columns = COLS;
	Rows = ROWS;

	void (*maze_algorithm)();
	maze_algorithm = &binary_tree_maze;

	if(argc >= 3) {
		int co = atoi(argv[1]);
		int ro = atoi(argv[2]);
		if(co>1 && ro>1) {
			Columns = co;
			Rows = ro;
		} else {
			die("Please provide parameters [columns]>1 and [rows]>1.", errno);
		}
	} else {
		die("./maze [columns] [rows]\n", errno);
	}

	Print_distances_flag = false;
	Print_path_flag = false;

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

					case 'a':
						maze_algorithm = &aldous_broder_maze;
						break;

					case 'd':
						Print_distances_flag = true;
						break;

#ifdef MAZE_TIGR
					case 'w':
						Draw_maze_flag = true;
						break;
#endif

					case 'p':
						Print_path_flag = true;

					default:
						die("   -s for sidewinder algorithm.\n   -d for distances\n", errno);
						break;
				}
			}
		}
	}

	// create the maze
	initialize();
	(*maze_algorithm)();
	
	// solve the maze
	Cell *max_distance_cell = calculate_distances(Grid[0]);

	// get closest path from south east corner
	Cell **breadcrumbs = path_to(max_distance_cell, max_distance_cell->distance);

	for(int i=max_distance_cell->distance; i>=0; i--) {
		printf("%d ", breadcrumbs[i]->distance);
	}
	printf("\n");

	// print to terminal
	size_t str_size = get_maze_string_size();
	char maze_str[str_size];
	to_string(maze_str, str_size, Print_distances_flag);
	printf("%s", maze_str);

	// draw to window
	if(Draw_maze_flag) draw(Grid, breadcrumbs, max_distance_cell->distance);

	if(Print_distances_flag) 
		printf("Max distance cell at column %d row %d, at distance %d steps.\n", 
			max_distance_cell->column+1, 
			max_distance_cell->row+1, 
			max_distance_cell->distance);
	
	// free and exit
	free(breadcrumbs);
	free_all();
	
	exit(EXIT_SUCCESS);
}

void initialize() {
	int cell_count = Columns * Rows;
	Grid = (Cell**)malloc(cell_count * sizeof(Cell*));
	if(!Grid) die("Failed to allocate memory for grid cells!", errno);
	for(int i=0; i<cell_count; i++) {
		Grid[i] = (Cell*) malloc(sizeof(Cell));
		if(!Grid[i]) die("Failed to allocate memory to cell.", errno);
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
	if(!c->links) die("Failed to allocate memory to cell links array.", errno);
	c->links_count = 0;
	c->distance = 0;
	c->solved = false;
	c->path = false;
}

void configure_cells() {
	if(!Grid) die("Grid not initilized.", errno);
	int cell_count = Columns * Rows;
	for(int i=0; i<cell_count; i++) {
		Cell *c = Grid[i];
		if(i>=Columns) c->north = Grid[i-Columns];
		if(row(i)<Rows-1) c->south = Grid[i+Columns];
		if(column(i)<Columns-1) c->east = Grid[i+1];
		if(column(i)>0) c->west = Grid[i-1];
	}
}

Cell *calculate_distances(Cell *root) {
	int max_cells = 64;
	Cell *front[max_cells];
	front[0] = root;
	int front_count = 1; // counting root as #1
	root->solved = true;
	Cell *max_distance_cell = root;
	while(front_count>0){
		if(front_count>max_cells) die("Maze too large for calculating distances.", errno);
		Cell *new_front[max_cells];
		int new_front_count = 0;

		for(int i=0; i<front_count; i++) {
			Cell *cell = front[i];
			for(int j=0; j<cell->links_count; j++) {
				Cell *linked = cell->links[j];
				if(linked->solved) continue; // if distance more than 0 skip
				linked->distance = cell->distance + 1;
				if(linked->distance > max_distance_cell->distance) 
					max_distance_cell = linked;
				linked->solved = true;
				new_front[new_front_count] = linked;
				new_front_count++;
			}
		}

		front_count = new_front_count;
		for(int k=0; k<new_front_count; k++) front[k] = new_front[k];
	}
	return max_distance_cell;
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
		if(!ca->links) die("Failed to increase links array size!", errno);
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

Cell** neighbors(Cell *c, int *counter) {
	if(MAZE_DEBUG) 
		printf("    Cell **neighbors(Cell *c)\n    Warning: Remember to free() return value.\n");

	Cell **neighbors = (Cell**) calloc(4, sizeof(Cell*));
	int i=0;
	if(c->north) neighbors[i++] = c->north;
	if(c->south) neighbors[i++] = c->south;
	if(c->east) neighbors[i++] = c->east;
	if(c->west) neighbors[i++] = c->west;
	*counter = i;
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

void aldous_broder_maze() {
	Cell *c = random_cell();
	int cells_count = Columns * Rows;
	int unvisited = cells_count -1;
	while(unvisited > 0) {
		int counter;
		Cell **neighbor_array = neighbors(c, &counter); 
		int rnd = random() % counter;
		Cell *n = neighbor_array[rnd];
		if(n->links_count==0) {
			link(c,n, true);
			unvisited -= 1;
		}
		c = n;
	}
}

// needs to free() return array
Cell **path_to(Cell *goal, int max_path) {
	if(!goal->solved) die("Trying to find closest path before solving maze.", errno);
	Cell *current = goal;
	Cell **breadcrumbs = (Cell**)malloc((max_path+1) * sizeof(Cell*));
	int breadcrumbs_counter = 0;
	breadcrumbs[breadcrumbs_counter++] = current;
	current->path = true;
	while(current->distance >= 0 && breadcrumbs_counter <= max_path) {
		int lowest = current->distance;
		Cell *candidate;
		for(int i=0; i<current->links_count; i++) {
			if(current->links[i]->distance < lowest) {
				lowest = current->links[i]->distance;
				candidate = current->links[i];
			}
		}
		breadcrumbs[breadcrumbs_counter++] = candidate;
		candidate->path = true;
		current = candidate;
	}
	return breadcrumbs;
}


//

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

void to_string(char str_out[], size_t str_size, bool print_distances) {

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
				if(print_distances) {
					if(c->path) sprintf(top_header, "%s%2d* ", top_header, c->distance);
					else sprintf(top_header, "%s%2d  ", top_header, c->distance);
				} else {
					strcpy(top_header, "    ");
				}
			} else {
				if(print_distances) {
					if(c->path) sprintf(top_header, "%s%2d*|", top_header, c->distance);
					else sprintf(top_header, "%s%2d |", top_header, c->distance);
				} else {
					strcpy(top_header, "   |");
				}
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

void draw(Cell **grid, Cell **breadcrumbs, int max_distance) {

	#ifdef MAZE_TIGR
	// ##############

	int win_width = 320;
	int win_height = 240;
	
	Tigr* screen = tigrWindow(win_width, win_height, "Maze", 0);
	
	int cell_size = 16;
	int half_cell_size = cell_size/2;
	
	int img_width = Columns * cell_size;
	int img_height = Rows * cell_size;
	
	int offx = (win_width-img_width)/2;
	int offy = (win_height-img_height)/2;

	int cell_count = Columns * Rows;
	
	while (!tigrClosed(screen) && !tigrKeyDown(screen, TK_ESCAPE)) {
		tigrClear(screen, White);
		// draw walls
		for(int i=0; i<cell_count; i++) {
			Cell *c = grid[i];
			int x1 = (column(i) * cell_size) + offx;
			int y1 = (row(i) * cell_size) + offy;
			int x2 = (column(i)+1) * cell_size + offx;
			int y2 = (row(i)+1) * cell_size + offy;
			tigrFillRect(screen, x1, y1, cell_size+2, cell_size+2, color_grid_distance(c, max_distance));
			if(!c->north) tigrLine(screen, x1,y1,x2,y1,Black); // north edge
			if(!c->west) tigrLine(screen, x1,y1,x1,y2,Black); // western edge
			if(!linked(c, c->east)) tigrLine(screen,x2,y1,x2,y2+1,Black);
			if(!linked(c, c->south)) tigrLine(screen,x1,y2,x2,y2,Black);
		}		
		// draw solution line
		int i=0;
		while(breadcrumbs[i+1] && breadcrumbs[i]->distance>0) {
			Cell *c = breadcrumbs[i];
			int x1 = (breadcrumbs[i]->column * cell_size) + half_cell_size + offx;
			int y1 = (breadcrumbs[i]->row * cell_size) + half_cell_size + offy;
			int x2 = (breadcrumbs[i+1]->column * cell_size) + half_cell_size + offx;
			int y2 = (breadcrumbs[i+1]->row * cell_size) + half_cell_size + offy;
			tigrLine(screen,x1,y1,x2,y2,Red);
			i++;
		}
		// print breadcrumb distances
		for(i=breadcrumbs[0]->distance; i>=0; i--) {
			int x1 = (breadcrumbs[i]->column * cell_size) + half_cell_size + offx;
			int y1 = (breadcrumbs[i]->row * cell_size) + half_cell_size + offy;
			char str[4];
			sprintf(str, "%d", breadcrumbs[i]->distance);
			int text_width_half = tigrTextWidth(tfont, str)/2;
			int text_height_half = tigrTextHeight(tfont, str)/2;
			tigrPrint(screen, tfont, x1-text_width_half, y1-text_height_half, tigrRGB(0xff, 0xff, 0xff), str);
		}
		tigrUpdate(screen);
	}

	tigrFree(screen);

	// ###########
	#endif

}

TPixel color_grid_distance(Cell *cell, int max) {
	if(!cell->solved) return White;
	float dist_f = (float)cell->distance;
	float max_f = (float)max;
	float intensity_f = (max_f - dist_f)/max_f;
	int dark = (int)(255.0 * intensity_f);
	int bright = 128 + (int)(127.0 * intensity_f);
	TPixel col = {dark, bright, dark, 255};
	return col;
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

void die(char *e, int n) {
	printf("%s %s %s", e, "Exiting program.\n", strerror(n));
	free_all();
	exit(EXIT_FAILURE);
}
