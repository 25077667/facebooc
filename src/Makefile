ifeq ($(strip $(CFLAGS)),)
CFLAGS = -std=gnu11 -Wall -Wextra -Werror -I
endif

ifeq ($(strip $(LDFLAGS)),)
LDFLAGS = -lsqlite3 -lpthread -ldl -lm
endif

SRC = $(shell ls *.c)
OBJS = $(SRC:%.c=%.o)
DEP = $(SRC:%.c=%.d)
FACEBOOC.a = facebooc.a

DIRS = $(shell ls -d ./*/ | sed 's,./\(.*\)/,\1,')
SUBLIB = $(shell echo $(DIRS) | tr ' ' '\n' | sed 's,\(.*\),\1/\1.a,')
INCLUDE = ../include

all: $(FACEBOOC.a)

$(FACEBOOC.a): $(SUBLIB) $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $^

./%.o: ./%.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEBUG) -c -MMD -MF $*.d $<

$(SUBLIB):
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir; \
	done

clean:
	$(RM) $(DEP) $(OBJS)
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir $@; \
	done

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEP)
endif