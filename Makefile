SOURCES=image_editor.c image.c
HEADERS=image.h utils.h
OBJECTS=image_editor.o image.o
EXE=image_editor

# C flags
CC=gcc
CFLAGS=-std=c99 -o
LDLIBS=-lm

ZIPNAME=C_image_editor.zip

.PHONY: build clean pack

build: image_editor

image_editor: $(OBJECTS)

image_editor.o: image_editor.c
	$(CC) $(CFLAGS) $@ $<

image.o: image.c
	$(CC) $(CFLAGS) $@ $<

clean:
	rm -f $(OBJECTS)
	rm -f $(EXE)
	rm -f $(ZIPNAME)

pack: clean
	zip -FSr $(ZIPNAME) $(SOURCES) $(HEADERS) Makefile README.md
