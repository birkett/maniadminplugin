# Make file to build libdemangle.a (Shamelessly taken from libiberty)
# Note that you don't need to run this in order for MAP to compile as
# it already links directly to the object files.

#Source Files
SRC_FILES = cp-demangle.cpp \
	    cp-demint.cpp \
	    cplus-dem.cpp \
	    safe-ctype.cpp \
	    xmalloc.cpp \
	    xstrdup.cpp 

OBJ_FILES = cp-demangle.o \
	    cp-demint.o \
	    cplus-dem.o \
	    safe-ctype.o \
	    xmalloc.o \
	    xstrdup.o 

COMP_VER=3.4
BINARY=test

CPLUS=/usr/local/gcc-3.4/bin/g++-$(COMP_VER)
CLINK=/usr/local/gcc-3.4/bin/g++-$(COMP_VER)

CFLAGS=-Wall

LDFLAGS=

all:
	rm -f -r *.a
	$(CPLUS) -I./ $(SRC_FILES) -c $(CFLAGS)
	ar -cvq libdemangle.a $(OBJ_FILES)
	rm -f -r *.o
clean:
	rm -f -r *.o
