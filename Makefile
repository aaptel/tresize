all: tresize

tresize: tresize.c
	gcc -Wall -o $@ $<
