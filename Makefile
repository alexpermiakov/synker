CC = gcc
CFALGS = -Wall -Wextra -Werror -I./src
SRCDIR = src
OBJDIR = obj

SRCS = $(shell find $(SRCDIR) -name "*.c")
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

TARGET = synker

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFALGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFALGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean