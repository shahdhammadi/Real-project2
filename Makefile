# Compiler and flags
CC = gcc
CFLAGS = -g -Wall -I./include
LDFLAGS = -lm

# Target executable
TARGET = rescue_simulation

# Source files
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = $(wildcard include/*.h)

# Default target
all: $(TARGET)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile source files to object files
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Debug with gdb
debug: $(TARGET)
	gdb ./$(TARGET)

# Show file list
files:
	@echo "Source files:"
	@ls -la $(SRC_DIR)/
	@echo "\nHeader files:"
	@ls -la include/

.PHONY: all clean run debug files