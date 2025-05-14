# This is our Makefile! It tells the computer how to build our game.

# We're using gcc as our C compiler
CC = gcc
# These are special options we give to gcc to make our code better and catch mistakes
CFLAGS = -Wall -Wextra -g -O2
# We need the ncurses library for our cool text-based graphics
LDFLAGS = -lncurses

# These are all the source files (.c files) that make up our game
SRCS = main.c data.c battlefield.c
# These are the object files (.o files) that gcc creates from our source files
OBJS = $(SRCS:.c=.o)
# This is the name of our game program when it's ready to play
TARGET = battle_arena

# These are special commands that aren't file names
.PHONY: all clean test

# The default goal - just type 'make' to build the game
all: $(TARGET)

# This tells make how to create our game program from the object files
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# This is a pattern rule that tells make how to create .o files from .c files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# This command cleans up all the files we created during building
clean:
	rm -f $(OBJS) $(TARGET)

# This helps us test our game to make sure it works correctly
test: $(TARGET)
	# First we'll test with AddressSanitizer to catch memory bugs
	$(CC) $(CFLAGS) -fsanitize=address $(SRCS) -o $(TARGET)_asan $(LDFLAGS)
	./$(TARGET)_asan
	rm $(TARGET)_asan
	
	# Then we'll use Valgrind to check for memory leaks
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) 