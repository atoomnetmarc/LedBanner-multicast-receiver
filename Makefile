CC = cc
PKG_CONFIG ?= pkg-config
CFLAGS = -Wall -Wextra -O2 $(shell $(PKG_CONFIG) --cflags SDL3 2>/dev/null || echo )
LDFLAGS = $(shell $(PKG_CONFIG) --libs SDL3 2>/dev/null || echo -lSDL3)

SRC = main.c display.c events.c multicast.c stats.c
OBJ = $(SRC:.c=.o)

all: led80x8

led80x8: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f led80x8 $(OBJ)

format:
	clang-format -i $(SRC) *.h