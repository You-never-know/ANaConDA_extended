CC = gcc
CFLAGS = -g -O0 -Wall -Wextra -std=c99
LFLAGS = -pthread
SRC = $(wildcard *.c)
EXEC = $(basename $(firstword $(SRC)))  # Extracts first C file's name (without extension)

OBJ = $(SRC:.c=.o)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
