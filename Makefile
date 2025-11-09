CC = cc
PKG_CONFIG ?= pkg-config
CFLAGS = -Wall -Wextra -O2 $(shell $(PKG_CONFIG) --cflags SDL3 2>/dev/null || echo )
LDFLAGS = $(shell $(PKG_CONFIG) --libs SDL3 2>/dev/null || echo -lSDL3)

SRC = main.c display.c events.c multicast.c stats.c
OBJ = $(SRC:.c=.o)

all: led80x8 gol_sender

led80x8: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

gol_sender: gol_sender.c config.h
	$(CC) $(CFLAGS) -o $@ gol_sender.c

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f led80x8 gol_sender $(OBJ)

format:
	clang-format -i $(SRC) *.h