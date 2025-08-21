CC      := $(CROSS_COMPILE)gcc
CFLAGS  := -Wall -Werror
TARGET  := writer
SRC     := writer.c
OBJ     := $(SRC:.c=.o)


# Default target
all: $(TARGET)

# Build writer from object file
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(TARGET) $(OBJ)
