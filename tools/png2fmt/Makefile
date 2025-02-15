CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lpng -lz
TARGET = png2fmt
SRC = png2fmt.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
