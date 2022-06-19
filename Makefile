CC       = gcc
CFLAGS   = -g -O2 -Wall -I. -Iutil $(shell sdl2-config --cflags)
LDFLAGS  = -lm -lSDL2 -lSDL2_ttf -lpthread

HDRS     = common.h util/util_misc.h util/util_sdl.h

CMB_SRC  = cmb.c graph.c sf.c util/util_sdl.c util/util_misc.c
CMB_OBJ  = $(CMB_SRC:.c=.o)

GALAXY_SRC  = galaxy.c graph.c sf.c util/util_sdl.c util/util_misc.c
GALAXY_OBJ  = $(GALAXY_SRC:.c=.o)

all: cmb galaxy

cmb: $(CMB_OBJ)
	$(CC) -o $@ $(CMB_OBJ) $(LDFLAGS)

galaxy: $(GALAXY_OBJ)
	$(CC) -o $@ $(GALAXY_OBJ) $(LDFLAGS)

$(CMB_OBJ) $(GALAXY_OBJ): $(HDRS)

clean:
	rm -f *.o util/*.o cmb galaxy

.PHONY: all
