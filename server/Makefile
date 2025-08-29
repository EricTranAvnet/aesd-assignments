CC      := $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g
TARGET = aesdsocket
SRC     := aesdsocket.c


all: $(TARGET)

default: all

$(TARGET): aesdsocket.c
	$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c

clean:
	rm -f $(TARGET)
