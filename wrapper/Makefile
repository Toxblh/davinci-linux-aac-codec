include ../.mk.defs

BASEDIR = ../
OBJDIR = $(BASEDIR)build

.PHONY: all

HEADERS = host_api.h plugin_api.h
SRCS = host_api.cpp plugin_api.cpp
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

all: $(HEADERS) $(SRCS) $(OBJS)

$(OBJDIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(OBJS)

