#include "maze.h"

#define COLS 8
#define ROWS 8
#define MAZE_DEBUG false
#define LINKS_SIZE_STEP 4
#define MAZE_TIGR
#define DEAD_END 1

static int Columns = COLS;
static int Rows = ROWS;

static Cell **Grid;

static bool Print_distances_flag = false;
static bool Draw_maze_flag = false;
static bool Print_path_flag = false;
static bool Performance_test_flag = false;
static bool Save_to_file_flag = false;
static bool Draw_live_flag = false;
static bool Print_dead_ends_flag = false;

Tigr* Window;

int main(int argc, char *argv[]) {

	void (*maze_algorithm)();
	maze_algorithm = &binary_tree_maze;

	if(argc == 1) die(" -b use binary algorithm (default)\n -s use sidewinder algorithm\n -a use [a]ldous broder algorithm\n -w use [w]ilson algorithm\n -h use [h[unt and kill algorithm\n-d print [d]istances\n -i draw fancy [i]mage in window using tigr\n -p [p]rint path\n -t performance [t]est\n -o save maze image to [o]utput file\n", errno);
	int arg_head = 1;
	if(argc >= 3) {
		int co = atoi(argv[arg_head]);
		int ro = atoi(argv[arg_head+1]);

		if(co>1 && ro>1) {
			Columns = co;
			Rows = ro;
			arg_head += 2;
		}
	}

	for( ; arg_head<argc; arg_head++) {
		char *argument = argv[arg_head];
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

				case 'w':
					maze_algorithm = &wilson_maze;
					break;

				case 'h':
					maze_algorithm = &hunt_and_kill;
					break;

				case 'd':
					Print_distances_flag = true;
					break;

				case 'p':
					Print_path_flag = true;
					break;

				case 'D':
					Print_dead_ends_flag = true;
					break;

				case 't':
					Performance_test_flag = true;
					break;

				case 'i':
					Draw_maze_flag = true;
					break;

				case 'o':
					Draw_maze_flag = true;
					Save_to_file_flag = true;
					break;

				case 'l':
					Draw_live_flag = true;
					break;

				default:
					die("Error, unknown argument.", errno);
					break;
			}
		} else {
			die("Error, unknown argument format.", errno);
		}
	}

	// create the maze
	initialize();

	srand(time(NULL));
	(*maze_algorithm)();
	
	// solve the maze
	Cell *max_distance_cell = calculate_distances(Grid[0]);

	// get closest path from south east corner
	// breadcrumbs is malloced – needs free()
	Cell **breadcrumbs = path_to(max_distance_cell, max_distance_cell->distance);

	if(Print_dead_ends_flag) printf("Dead ends: %d\n", dead_ends());

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

	
	if(Performance_test_flag) {
		int test_runs = 1000;
		unsigned long binary_time = (unsigned long)performance_test(&binary_tree_maze, test_runs);
		unsigned long sidewinder_time = (unsigned long)performance_test(&sidewinder_maze, test_runs);
		unsigned long aldous_broder_time = (unsigned long)performance_test(&aldous_broder_maze, test_runs);
		printf("    testing algorithms %d runs, size %d x %d\n    binary = %lu ms\n    sidewinder = %lu ms\n    aldous broder = %lu ms\n", test_runs, Columns, Rows, binary_time, sidewinder_time, aldous_broder_time);
	}
	

end:
	// free and exit
	free(breadcrumbs);
	free_all();
	draw_end();
	exit(EXIT_SUCCESS);
}

// initialize allocate memory (malloc) for the array contaning the main grid of cells
void initialize() {
	int cell_count = size();
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

// init_cell allocate memory (calloc) for each cells links array
void init_cell(Cell *c, int column, int row) {
	c->column = column;
	c->row = row;
	c->links_size = LINKS_SIZE_STEP;
	c->links = (Cell **)calloc(c->links_size, sizeof(Cell*));
	if(!c->links) die("Failed to allocate memory to cell links array.", errno);
	c->links_count = 0;
	c->distance = 0;
	c->marker = ' ';
	c->solved = false;
	c->path = false;
}

void configure_cells() {
	if(!Grid) die("Grid not initilized.", errno);
	int cell_count = size();
	int neighbors = 0;
	for(int i=0; i<cell_count; i++) {
		Cell *c = Grid[i];
		if(i>=Columns) c->north = Grid[i-Columns];
		else c->north = NULL;
		if(row(i)<Rows-1) c->south = Grid[i+Columns];
		else c->south = NULL;
		if(column(i)<Columns-1) c->east = Grid[i+1];
		else c->east = NULL;
		if(column(i)>0) c->west = Grid[i-1];
		else c->west = NULL;
	}
}

Cell *cell(int column, int row) {
	if(column < 0 || column >= Columns) return NULL;
	if(row < 0 || row >= Rows) return NULL;
	return Grid[index_at(column, row)];
}

void link_cells(Cell *ca, Cell *cb, bool is_bidi) {
	if(ca->links_count >= ca->links_size) {
		ca->links_size = ca->links_size + LINKS_SIZE_STEP;
		ca->links = (Cell **)realloc(ca->links, ca->links_size * sizeof(Cell*));
		if(!ca->links) die("Failed to increase links array size!", errno);
	}
	if(!linked(ca,cb)) {
		ca->links[ca->links_count] = cb;
		ca->links_count++;
	}
	if(is_bidi) link_cells(cb, ca, false);
}

bool unlink_cells(Cell *ca, Cell *cb, bool is_bidi) {
	bool is_found = false;
	for(int i=0; i<ca->links_count; i++) {
		if(!is_found)
			if(ca->links[i] == cb) is_found = true;
		if(is_found) 
			ca->links[i] = ca->links[i+1];
	}
	if(is_found) ca->links_count--;
	if(is_bidi) unlink_cells(cb, ca, false);
	return is_found;
}

bool linked(Cell *ca, Cell *cb) {
	if(ca && cb) return(find_link(ca,cb)>0);
	return false;
}

bool find_link(Cell *ca, Cell *cb) {
	if(ca && cb) 
		for(int i=0; i<ca->links_count; i++) 
			if(ca->links[i] == cb) return true;
	return false;
}

Cell** links(Cell *c) {
	return c->links;
}

// warning: caller expected to free returned malloc array
Cell **neighbors(Cell *c, int *counter) {
	if(MAZE_DEBUG) 
		printf("    Cell **neighbors(Cell *c)\n    Warning: Remember to free() return value.\n");

	Cell **neighbors = (Cell**) calloc(4, sizeof(Cell*));
	if(!neighbors) die("Failed allocating memory for neighbors array.", errno);
	int i=0;
	if(c->north) neighbors[i++] = c->north;
	if(c->south) neighbors[i++] = c->south;
	if(c->east) neighbors[i++] = c->east;
	if(c->west) neighbors[i++] = c->west;
	*counter = i;
	return neighbors;
}

// warning: caller expected to free returned malloc array
Cell **neighbors_unlinked(Cell *c, int *counter) {
	Cell **arr = (Cell**) calloc(4, sizeof(Cell*));
	if(!arr) die("Failed allocating memory for neighbors array.", errno);
	int i=0;
	if(c->north && !linked(c, c->north)) arr[i++] = c->north;
	if(c->south && !linked(c, c->south)) arr[i++] = c->south;
	if(c->east  && !linked(c, c->east)) arr[i++] = c->east;
	if(c->west  && !linked(c, c->west)) arr[i++] = c->west;
	*counter = i;
	return arr;
}

int neighbors_count(Cell *c) {
	int n=0;
	if(c->north) n++;
	if(c->south) n++;
	if(c->east) n++;
	if(c->west) n++;
	return n;
}

Cell *get_random_neighbor(Cell *c) {
	int counter;
	Cell **neighbor_array = neighbors(c, &counter);
	int rnd = rand() % counter;
	Cell *random_neighbor = neighbor_array[rnd];
	free(neighbor_array);
	return random_neighbor;
}

Cell *get_random_neighbor_without_link(Cell *c) {
	int counter;
	Cell **unlinked = neighbors_unlinked(c, &counter);
	Cell *random_cell = NULL;
	if(counter > 0) {
		Cell *free_cells[counter];
		int head = 0;
		for(int i=0; i<counter; i++) {
			if(unlinked[i]->links_count==0) free_cells[head++] = unlinked[i];
		}
		if(head>0) {
			int rnd = random() % head;
			random_cell = free_cells[rnd];
		}
	}
	if(unlinked) free(unlinked);
	return random_cell;
}

// ### algorithms

// -b (default)
void binary_tree_maze() {
	int cell_count = size();
	if(Draw_live_flag) draw_start();
	for(int i=0; i<cell_count; i++) {
		Cell *c = Grid[i];
		int j = 0;
		Cell *neighbors[2];
		for(int n=0;n<2;n++) neighbors[n] = NULL;
		if(c->north) neighbors[j++] = c->north;
		if(c->east) neighbors[j++] = c->east;
		if(j==0) continue;
		//srand(time(NULL));
		int rnd = rand() % j;
		Cell *neighbor = neighbors[rnd];
		link_cells(c, neighbor, true);
		if(Draw_live_flag) draw_update(10);
	}
}

// -s
void sidewinder_maze() { 
	if(Draw_live_flag) draw_start();
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
				if(member->north) link_cells(member, member->north, true);
				while(index > 0) {
					index--;
					corridor[index] = NULL;
				}
			} else {
				link_cells(c, c->east, true);
				if(Draw_live_flag) draw_update(10);
			}
		}
	}
}

// -s
void aldous_broder_maze() { 
	Cell *c = random_cell_from_grid(NULL);
	int cell_count = size();
	int unvisited = cell_count -1;
	if(Draw_live_flag) draw_start();
	while(unvisited > 0) {
		int counter;
		Cell **neighbor_array = neighbors(c, &counter); 
		int rnd = random() % counter;
		Cell *n = neighbor_array[rnd];
		free(neighbor_array);
		if(n->links_count==0) {
			link_cells(c,n, true);
			if(Draw_live_flag) draw_update(10);
			unvisited -= 1;
		}
		c = n;
	}
}

// -w
void wilson_maze() { 
	int cell_count = size();
	int unvisited_length = cell_count;
	Cell *unvisited[unvisited_length];
	for(int c=0; c<unvisited_length; c++) unvisited[c] = Grid[c];
	int cell_index;
	Cell *first = random_cell_from_array(unvisited, unvisited_length, &cell_index);
	unvisited_length = remove_cell_from_array(unvisited, cell_index, unvisited_length);
	if(Draw_live_flag) draw_start();

	while(unvisited_length>0) {	
		Cell *cell = random_cell_from_array(unvisited, unvisited_length, &cell_index);
		Cell *path[cell_count];
		for(int i=0; i<cell_count; i++) path[i] = NULL;
		int path_length = 0;
		path[0] = cell;
		path_length++;

		while(array_includes_cell(unvisited, cell, unvisited_length, NULL)) {
			cell = get_random_neighbor(cell);
			int position;
			array_includes_cell(path, cell, path_length, &position);
			if(position >= 0) {
				path_length = position+1;
			} else {
				path[path_length] = cell;
				path_length++;
			}
		}

		for(int i=0; i<path_length-1; i++) {
			link_cells(path[i], path[i+1], true);
			if(Draw_live_flag) draw_update(10);
			int index;
			array_includes_cell(unvisited, path[i], unvisited_length, &index);
			if(index >= 0) unvisited_length = remove_cell_from_array(unvisited, index, unvisited_length);
		}
	}
}

// -h
void hunt_and_kill() {
	int cell_count = size();
	Cell *c = random_cell_from_grid(NULL);
	int unvisited = cell_count-1;
	bool kill_mode = true;
	
	enum MODE {kill, hunt};
	enum MODE mode = kill;

	while(unvisited > 0) {
		switch(mode) {
			case kill: {
				Cell *l = NULL;
				l = get_random_neighbor_without_link(c);
				if(!l) {
					mode = hunt;
				} else {
					link_cells(c, l, true);
					unvisited--;
					c = l;
				}
				break;
			}
			case hunt: {
				for(int i=0; i<cell_count; i++) {
					c = Grid[i];
					if(c->links_count > 0) continue;
					int cnt;
					Cell **arr = neighbors(c, &cnt);
					Cell *arr_lnk[cnt];
					int head = 0;
					for(int j=0; j<cnt; j++) {
						if(arr[j]->links_count>0) arr_lnk[head++] = arr[j];					
					}
					if(arr) free(arr);
					if(head>0){
						int rnd = random() % head;
						link_cells(c, arr_lnk[rnd], true);
						c = arr_lnk[rnd];
						unvisited--;
						mode = kill;
						break;
					}
				}
				break;
			}
		}
	
	}
}


// ### end algorithms



bool array_includes_cell(Cell *arr[], Cell *c, int arr_len, int *index) {
	for(int i=0; i<arr_len; i++) {
		if(arr[i] == c) {
			if(index) *index = i;
			return true;
		}
	}
	if(index) *index = -1;
	return false;
}

int remove_cell_from_array(Cell *arr[], int cell_index, int length) {
	for(int i=cell_index; i<length; i++) {
		if(i==length-1) arr[i]=NULL;
		else arr[i]=arr[i+1];
	}
	length--;
	return length;
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

int dead_ends() {
	int cell_count = Columns * Rows;
	int deads = 0;
	for(int i=0; i<cell_count; i++) {
		Cell *c = Grid[i];
		if(c->links_count == DEAD_END) {
			c->marker = '*';
			deads++;
		}
	}
	return deads;
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

// return maze size
int size() {
	return Columns * Rows;
}

Cell *random_cell_from_grid(int *index) {
	int r = rand() % size();
	if(index) *index = r;
	return Grid[r];
}

Cell *random_cell_from_array(Cell **array, int length, int *index) {
	if(!array) {
		array = Grid;
		length = size();
	}
	int r = rand() % length;
	if(index) *index = r;
	return array[r];
}

void clear_maze_links() {
	int s = size();
	for(int i=0; i<s; i++) {
		Cell *c = Grid[i];
		for(int l=0;l<c->links_count;l++) {
			c->links[l] = NULL;
		}
		c->links_count = 0;
	}
}

clock_t performance_test(void (*alg)(), int runs) {
	clock_t t = clock();
	for(int i=0; i<runs; i++) {
		(*alg)();
		clear_maze_links();
	}
	clock_t time_passed = clock() - t;
	return time_passed;
}



// ### output

// warning: caller expected to free malloced array

size_t get_maze_string_size() {
	size_t str_size = (Columns * 4 + 1) * (Rows * 2 + 1);
	str_size += Rows * 2; // for newlines, 2 newlines for each row
	str_size += 1;	      // for '\0'
	return str_size;
}

// warning: caller expected to free returned malloced array
Cell **path_to(Cell *goal, int max_path) {
	if(!goal->solved) die("Trying to find closest path before solving maze.", errno);
	Cell *current = goal;
	Cell **breadcrumbs = (Cell**)malloc((max_path+1) * sizeof(Cell*));
	if(!breadcrumbs) die("Failed to allocate memory for breadcrumbs array.", errno);
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
					//strcpy(top_header, "    ");
					sprintf(top_header, "%s %c  ", top_header, c->marker);
				}
			} else {
				if(print_distances) {
					if(c->path) sprintf(top_header, "%s%2d*|", top_header, c->distance);
					else sprintf(top_header, "%s%2d |", top_header, c->distance);
				} else {
					//strcpy(top_header, "   |");
					sprintf(top_header, "%s %c |", top_header, c->marker);
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

void draw_start() {
#ifdef MAZE_TIGR

	int win_width = 320;
	int win_height = 240;
	Window = tigrWindow(win_width, win_height, "Maze", 0);
	if(!Window) die("Failed to create tigrWindow.", errno);

#endif
}

void draw_update(int slow) {
#ifdef MAZE_TIGR

	int cell_count = size();
	int cell_size = 16;
	int win_width = 320;
	int win_height = 240;
	int half_cell_size = cell_size/2;
	int img_width = Columns * cell_size;
	int img_height = Rows * cell_size;
	int offx = (win_width-img_width)/2;
	int offy = (win_height-img_height)/2;
	
	tigrClear(Window, White);
	for(int i=0; i<cell_count; i++) {
		Cell *c = Grid[i];
		int x1 = (column(i) * cell_size) + offx;
		int y1 = (row(i) * cell_size) + offy;
		int x2 = (column(i)+1) * cell_size + offx;
		int y2 = (row(i)+1) * cell_size + offy;
		if(!c->north) tigrLine(Window, x1,y1,x2,y1,Black); // north edge
		if(!c->west) tigrLine(Window, x1,y1,x1,y2,Black); // western edge
		if(!linked(c, c->east)) tigrLine(Window,x2,y1,x2,y2+1,Black);
		if(!linked(c, c->south)) tigrLine(Window,x1,y2,x2,y2,Black);
	}
	tigrUpdate(Window);
	usleep(slow * 10000);

#endif
}

void draw_end() {
#ifdef MAZE_TIGR

	if(!Window) return;
	while (!tigrClosed(Window) && !tigrKeyDown(Window, TK_ESCAPE)) { 
		//usleep(1*100000);
		tigrPrint(Window, tfont, 10, 10, tigrRGB(0xff, 0xff, 0xff), "Done");
		tigrUpdate(Window);
	}
	if(Window) tigrFree(Window);
	
#endif
}

void draw(Cell **grid, Cell **breadcrumbs, int max_distance) {
#ifdef MAZE_TIGR

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
	bool is_not_saved = true;
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
		if(Save_to_file_flag && is_not_saved) {
			printf("Saving file ...\n");
			int save_check = tigrSaveImage("./maze_image.png", screen);
			if(save_check == 0) die("Failed to save image to file.", errno);
			is_not_saved = false;
		}
	}

	tigrFree(screen);

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

// end output



void free_all() {
	if(!Grid) return;
	int cell_count = size();
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
