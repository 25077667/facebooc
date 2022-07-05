ifeq ($(strip $(CFLAGS)),)
CFLAGS = -std=c11 -Wall -Wextra -Werror
endif

ifeq ($(strip $(LDFLAGS)),)
CFLAGS = -lsqlite3 -lpthread -ldl -lm
endif

DEBUG = -DDEBUG -g -fsanitize=address

SRC = $(shell find . -type f -name "*.[ch]")
OBJ = $(patsubst %.c,%.o, $(filter %.c, $(SRC)))

.PHONY: all clean test

all: $(OBJ)

http/%.o: http/%.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(DEBUG) -o $@ $<

clean:
	$(RM) $(OBJ)

test: $(OBJ)
	@echo Do testing...
	@./driver.py
	@echo done