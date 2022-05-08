CC       = gcc
CFLAGS   = -g -O2 -Wall -I. -Iutil
LDFLAGS  = -lm -lSDL2 -lSDL2_ttf -lpthread

TARGET   = hubble
SOURCES  = main.c sf.c display.c \
           util/util_sdl.c util/util_misc.c

util/util_sdl.o: CFLAGS += $(shell sdl2-config --cflags)

OBJ := $(SOURCES:.c=.o)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)

