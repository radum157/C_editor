SOURCES=image_editor.c image.c
HEADERS=image.h utils.h
OBJECTS=image_editor.o image.o
EXE=image_editor

CC=gcc
CFLAGS=-std=c99 -g -c -Wall -Wextra -o
LDLIBS=-lm

# Clear to disable valgrind
RUNCC=valgrind
VALGOPT=--leak-check=full --show-leak-kinds=all --track-origins=yes -s
DEBUGCC=gdb

ZIPNAME=323CA_MarinRadu_Tema3.zip

.PHONY: build clean pack run all debug

build: image_editor

image_editor: $(OBJECTS)

image_editor.o: image_editor.c
	$(CC) $(CFLAGS) $@ $<

image.o: image.c
	$(CC) $(CFLAGS) $@ $<

run:
	$(RUNCC) $(VALGOPT) ./$(EXE)

debug: clean build
	$(DEBUGCC) $(EXE)

all: clean build
	make run
	make clean

clean:
	rm -f $(OBJECTS)
	rm -f $(EXE)
	rm -f $(ZIPNAME)

pack: clean
	zip -FSr $(ZIPNAME) $(SOURCES) $(HEADERS) Makefile README.md
