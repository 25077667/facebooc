CFLAGS = -std=gnu11 -Wall -Wextra -Werror -I
LDFLAGS = -lsqlite3 -lpthread -ldl -lm -lc -lssl -lcrypto
INCLUDE = include

export CFLAGS LDFLAGS DEBUG
GIT_HOOKS := .git/hooks/applied

OUT_DIR = bin
EXEC = $(OUT_DIR)/facebooc
SRC = $(shell find src/ -type f -name "*.[ch]") \
	$(shell find include/ -type f -name "*.[ch]")
OBJS = $(patsubst %.c,%.o, $(filter %.c, $(SRC)))
STATICS = static/css/main.css templates/version.html

all: $(GIT_HOOKS) $(EXEC) $(STATICS)

$(EXEC): $(OBJS) main.c
	mkdir -p $(OUT_DIR)
	$(CC) $(CFLAGS) $(INCLUDE) $(DEBUG) $(RELEASE) \
	-o $@ main.c $(OBJS) $(LDFLAGS)

$(OBJS): $(SRC)
	@echo $(OBJS)
	$(MAKE) -C src

TEST_FILES = $(shell find tests/ -type f -name "*.c")
test: $(OBJS) $(TEST_FILES) 
	$(MAKE) -C tests $@

$(STATICS):
	$(MAKE) -C static

run: $(EXEC) $(STATICS)
	@echo "Starting Facebooc service..."
	@./$(EXEC) $(port)

release: $(EXEC) $(STATICS)

format:
	@echo start formatting...
	@for f in $(SRC); do \
		clang-format -i $$f ; \
	done
	@echo finish format!

clean:
	$(RM) $(EXEC)
	@for dir in $(shell ls */Makefile | grep -o ".*/"); do \
		$(MAKE) -C $$dir $@; \
	done
	$(RM) templates/version.html

distclean: clean
	$(RM) db.sqlite3

$(GIT_HOOKS):
	@scripts/install-git-hooks
	@echo

ifneq "$(MAKECMDGOALS)" "release"
DEBUG = -DDEBUG -g -fsanitize=address
LDFLAGS += -lasan
else
DEBUG =
RELEASE = -O3 -s -flto
endif