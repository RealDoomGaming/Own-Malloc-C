# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -I.

# Directories
MM_DIR = MemoryManagment

# Source files
SOURCES = main.c $(MM_DIR)/mm.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Output executable
TARGET = mallocTest

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJECTS) $(TARGET)
