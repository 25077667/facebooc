ifeq ($(strip $(CFLAGS)),)
CFLAGS = -std=gnu11 -Wall -Wextra -Werror -I
endif

SRC = $(shell ls *.c)
OBJS = $(SRC:%.c=%.o)
DEP = $(SRC:%.c=%.d)
TARGET.a = models.a

INCLUDE = ../../include/models -I ../../include

all: $(TARGET.a)

$(TARGET.a): $(OBJS)
	$(LD) -o $@ $(LDFLAGS) $^

./%.o: ./%.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEBUG) -c -MMD -MF $*.d $<

clean:
	$(RM) $(DEP) $(OBJS) $(TARGET.a)

ifneq "$(MAKECMDGOALS)" "clean"
-include $(DEP)
endif