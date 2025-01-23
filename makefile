# Very simple makefile for imcsh
CC = gcc

imcsh: imcsh.c
	$(CC) imcsh.c -o imcsh

clean:
	rm -f *.o imcsh

run: imcsh
	./imcsh