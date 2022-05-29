CC       = gcc
CFLAGS   = -g -O2 -Wall -I. -Iutil $(shell sdl2-config --cflags)
LDFLAGS  = -lm -lSDL2 -lSDL2_ttf -lpthread

TARGET   = cmb
SOURCES  = cmb.c sf.c \
           util/util_sdl.c util/util_misc.c

OBJ := $(SOURCES:.c=.o)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)

