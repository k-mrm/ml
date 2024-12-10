CC := gcc
CFLAGS= -lm -std=c11 -O2 -g -DNDEBUG
SRCROOT = .
SRCDIRS := $(shell find $(SRCROOT) -type d)
SRCS=$(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))
OBJS=$(SRCS:.c=.o)
TARGET := ml
.PHONY: test clean

run: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

debug: $(OBJS)
	$(CC) -o $(TARGET) -g -O0 -DNDEBUG $(OBJS) $(LDFLAGS) $(CFLAGS)

perf: $(OBJS)
	$(CC) -o $(TARGET) -g -Og -DNDEBUG $(OBJS) $(LDFLAGS) $(CFLAGS)

clean:
	$(RM) $(OBJS)
