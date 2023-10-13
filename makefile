# Set variables
CC = cc
CFLAGS = -g
SRCS = $(wildcard src/*.c) $(wildcard src/level-1/*.c) $(wildcard src/level-2/*.c)


# Default target
build: diskimage $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o aydensEXT2

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Download diskimage
diskimage:
	rm -f diskimage
	sudo wget https://eecs.wsu.edu/~cs360/samples/PROJECT/disk2 -O diskimage
	sudo chmod 777 diskimage

# Clean up
clean:
	rm -f $(OBJS) aydensEXT2 diskimage