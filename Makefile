default: a.out

a.out: main.c
	gcc -g main.c -o a.out

.PHONY: debug clean run

debug: a.out
	gdb ./a.out

clean:
	rm -f *.out

run: a.out
	./a.out
