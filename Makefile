CFLAGS = -std=c11 -Wall -Wextra -Werror -I include
LDFLAGS = -lsqlite3 -lpthread -ldl -lm

ifeq ($(DEBUG), off)
DEBUG_FLAG =
else
DEBUG_FLAG = -DDEBUG -g
endif

# strtok_r is provided by POSIX.1c-1995 and POSIX.1i-1995, however, with
# the POSIX_C_SOURCE=1 on Mac OS X is corresponding to the version of
# 1988 which is too old (defined in sys/cdefs.h)
CFLAGS += -D_POSIX_C_SOURCE=200809L

UNAME_S = $(shell uname -s)
GIT_HOOKS := .git/hooks/applied

OUT = bin
EXEC = $(OUT)/facebooc
OBJS = \
	$(patsubst src/%.c,src/%.o, $(wildcard src/*.c) \
								$(wildcard src/models/*.c) \
								$(wildcard src/http/*.c))

TEST_UNIT_OBJ = \
	$(patsubst tests/%.c,tests/%.o, $(wildcard tests/http/*.c))

TEST_UNIT = $(wildcard tests/http/*.o)

c_codes := $(wildcard include/*.h include/models/*.h src/*.c src/models/*.c tests/http/*.c)

scss_dir = static/scss/
css_dir = static/css/

deps := $(OBJS:%.o=%.o.d)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(DEBUG_FLAG) -o $@ -MMD -MF $@.d -c $<

all: $(GIT_HOOKS) $(EXEC) main.c

$(EXEC): $(OBJS)
	mkdir -p $(OUT)
	$(CC) $(CFLAGS) main.c $(OBJS) -fsanitize=address -g -o $@ $(LDFLAGS)

html-updater: $(EXEC)
	@scripts/auto-update-html.sh

gen-css:
	mkdir -p static/css/
	sassc ./static/scss/main.scss ./static/css/main.css

run: $(EXEC) html-updater gen-css
	@echo "Starting Facebooc service..."
	@./$(EXEC) $(port)

before_release: $(OBJS)
	mkdir -p $(OUT)
	$(CC) $(CFLAGS) -O3 -s -o $(EXEC) main.c $(OBJS) $(LDFLAGS)

tests/%.o: tests/%.c $(OBJS)
	$(CC) $(CFLAGS) $< $(OBJS) -fsanitize=address -g -o $@ $(LDFLAGS)

test: $(TEST_UNIT_OBJ)
	@echo Do testing...
	@tests/driver.py
	@echo done

_debud_off:
	@$(MAKE) _actual_release DEBUG=off

_actual_release: before_release html-updater gen-css

release: _debud_off

format:
	@echo start formatting...
	@for f in $(c_codes); do \
		clang-format -i $$f ; \
	done
	@echo finish format!

clean:
	$(RM) $(OBJS) $(TEST_UNIT) $(TEST_UNIT_OBJ) $(EXEC) $(deps)
	$(RM) templates/version.html

distclean: clean
	$(RM) db.sqlite3

$(GIT_HOOKS): format
	@scripts/install-git-hooks
	@echo

-include $(deps)
