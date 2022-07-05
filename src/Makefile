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
SUBLIB = $(patsubst %.c,%.o, $(shell find ./*/ -type f -name "*.c"))
INCLUDE = ../include

all: $(SUBLIB) $(OBJS)

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