
all: print

print: print.c
	gcc -O2 print.c -Wall -o print

