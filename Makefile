
CC=mpic++
CFLAGS=-fPIC -std=c++11 -Wall -g -O0 -Iinclude
LDFLAGS=-lnetcdf_c++4 -lyaml-cpp

BUILDDIR=build

SRCS=$(wildcard lib/*.cc)
OBJS=$(patsubst lib/%.cc,build/%.o,$(SRCS))

all: dir $(BUILDDIR)/libtango.so $(BUILDDIR)/libtango.a

dir:
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/libtango.so: $(OBJS)
	$(CC) -shared $^ -o $@ $(LDFLAGS)

$(BUILDDIR)/libtango.a: $(OBJS)
	ar rcs $@ $^

$(OBJS): $(BUILDDIR)/%.o : lib/%.cc
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -rf $(BUILDDIR)
