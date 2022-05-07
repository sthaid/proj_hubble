CC       = gcc
CFLAGS   = -g -O2 -Wall -I.
LDFLAGS  = -lm 

TARGET   = hubble
SOURCES  = main.c sf.c

OBJ := $(SOURCES:.c=.o)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)

