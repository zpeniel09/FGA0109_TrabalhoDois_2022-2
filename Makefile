# Executavel
BINFOLDER := bin/
# .h
INCFOLDER := inc/
# .c
SRCFOLDER := src/
# .o
OBJFOLDER := obj/
CC := gcc
CFLAGS := -std=c99
LINKFLAGS := -lm -lwiringPi
SRCFILES := $(wildcard src/*.c)

all: $(SRCFILES:src/%.c=obj/%.o)
	$(CC) $(CFLAGS) obj/*.o -o bin/prog $(LINKFLAGS)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -I./inc $(LINKFLAGS)

run: bin/prog
	bin/prog

.PHONY: clean
clean:
	rm -rf obj/*
	rm -rf bin/*
	touch obj/.gitkeep
	touch bin/.gitkeep