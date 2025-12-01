CC = gcc
CFLAGS = -std=c99 -ggdb -Wall -Wextra -pedantic

OBJ = interp.o parser.o stack.o symtab.o tree_node.o

# Default target
all: interp

# Build the interpreter executable
interp: $(OBJ)
	$(CC) $(CFLAGS) -o interp $(OBJ)

# Compile individual source files
interp.o: interp.c interp.h parser.h stack.h stack_node.h symtab.h tree_node.h
	$(CC) $(CFLAGS) -c interp.c

parser.o: parser.c parser.h interp.h stack.h stack_node.h symtab.h tree_node.h
	$(CC) $(CFLAGS) -c parser.c

stack.o: stack.c stack.h stack_node.h
	$(CC) $(CFLAGS) -c stack.c

symtab.o: symtab.c symtab.h
	$(CC) $(CFLAGS) -c symtab.c

tree_node.o: tree_node.c tree_node.h symtab.h
	$(CC) $(CFLAGS) -c tree_node.c

# Clean object files and executable
clean:
	rm -f *.o interp

# Clean everything (same as clean here)
realclean: clean
