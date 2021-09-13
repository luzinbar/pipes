all: exec

libs: c-lib


c-lib: task2.c
	gcc -g -m32 -c -o task2.o task2.c

exec: task2.c libs
	gcc -g -m32 -c -o task2.o task2.c
	gcc -g -m32  task2.o -o main
	rm task2.o

.PHONY: clean
clean:
	rm -rf ./*.o main