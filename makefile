# Very simple makefile for imcsh
CC = gcc

imcsh: imcsh.o
	$(CC) imcsh.o -o imcsh 

imcsh.o: imcsh.c
	$(CC) -c imcsh.c 

clean:
	rm -f *.o imcsh

run:
	./imcsh