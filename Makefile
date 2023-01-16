CFLAGS = -I/tigr
ifeq ($(OS),Windows_NT)
	LDFLAGS = -s -lopengl32 -lgdi32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS = -framework OpenGL -framework Cocoa
	else ifeq ($(UNAME_S),Linux)
		LDFLAGS = -s -lGLU -lGL -lX11
	endif
endif

maze : maze.c tigr/tigr.c
	gcc $^ -Os -o $@ $(CFLAGS) $(LDFLAGS)

mazed : maze.c tigr/tigr.c
	gcc $^ -O0 -g -o $@ $(CFLAGS) $(LDFLAGS)

clean :
	rm -f maze mazed