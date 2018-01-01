
all: print

print: print.c
	gcc -g -O2 print.c -Wall -o print

