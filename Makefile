include .mk.defs

BASEDIR = ./

BUILD_DIR = .
SUBDIRS = wrapper

ifeq ($(OS_TYPE), Linux)
LDFLAGS = -shared '-Wl,-rpath,$$ORIGIN' -Wl,-z,origin -lpthread -stdlib=libc++ -lavcodec -lavutil
else
LDFLAGS = -dynamiclib
endif

TARGET = $(BINDIR)/aac_encoder_plugin.dvcp

OBJDIR = $(BUILD_DIR)/build
BINDIR = $(BUILD_DIR)/bin

.PHONY: all

HEADERS = plugin.h audio_encoder.h
SRCS = plugin.cpp audio_encoder.cpp
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

all: prereq make-subdirs $(HEADERS) $(SRCS) $(OBJS) $(TARGET)

prereq:
	mkdir -p $(OBJDIR)
	mkdir -p $(BINDIR)

$(OBJDIR)/%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET):
	$(CC) $(OBJDIR)/*.o $(LDFLAGS) -o $(TARGET)
	install -Dm755 $(TARGET) /opt/resolve/IOPlugins/aac_encoder_plugin.dvcp.bundle/Contents/Linux-x86-64/aac_encoder_plugin.dvcp

clean: clean-subdirs
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)

make-subdirs:
	@for subdir in $(SUBDIRS); do \
	echo "Making $$subdir"; \
	(cd $$subdir; make; cd ..) \
	done

clean-subdirs:
	@for subdir in $(SUBDIRS); do \
	echo "Making clean in $$subdir"; \
	(cd $$subdir; make clean; cd ..) \
	done
