all: exec

libs: c-lib


c-lib: task3.c
	gcc -g -m32 -c -o task3.o task3.c

exec: task3.c libs
	gcc -g -m32 -c -o task3.o task3.c
	gcc -g -m32  task3.o -o main
	rm task3.o

.PHONY: clean
clean:
	rm -rf ./*.o main