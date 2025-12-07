# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -IMemoryManagment/UsingChunck

# Directories
CHUNK_DIR = MemoryManagment/UsingChunck

# Source files
SRCS = main.c $(CHUNK_DIR)/mm.c

# Object files
OBJS = main.o $(CHUNK_DIR)/mm.o

# Executable
EXEC = program

# Default target
all: $(EXEC)

# Build executable
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile main.c
main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile mm.c
$(CHUNK_DIR)/mm.o: $(CHUNK_DIR)/mm.c $(CHUNK_DIR)/mm.h
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(EXEC)
