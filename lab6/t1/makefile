all: exec

libs: c-lib


c-lib: task1.c
	gcc -g -m32 -c -o task1.o task1.c

exec: task1.c libs
	gcc -g -m32 -c -o task1.o task1.c
	gcc -g -m32  task1.o -o main
	rm task1.o

.PHONY: clean
clean:
	rm -rf ./*.o main