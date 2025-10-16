default: a.out

a.out: main.c
	gcc -fsanitize=address -g main.c -o a.out

.PHONY: debug clean run valgrind

valgrind: a.out
	valgrind --leak-check=full --track-origins=yes ./a.out

bench: a.out
	valgrind --tool=massif ./a.out

debug: a.out
	gdb ./a.out

clean:
	rm -f *.out

run: a.out
	./a.out
