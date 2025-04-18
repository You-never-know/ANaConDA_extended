CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -std=c99
SRC = $(wildcard *.c)
EXEC = $(basename $(firstword $(SRC)))  # Extracts first C file's name (without extension)

OBJ = $(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
