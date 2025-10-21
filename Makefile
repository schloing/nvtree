CC_FLAGS = -Wall -O2
OBJ = main.o

.PHONY: clean run debug massif test

libnvtree.a: $(OBJ)
	ar rcs $@ $^

main.o: main.c nvtree.h
	gcc $(CC_FLAGS) -c main.c -o $@

test.o: test/main.c $(OBJ)
	gcc -fsanitize=address $(CC_FLAGS) $^ -o $@

test: test.o
	./test.o

massif: test
	valgrind --tool=massif --massif-out-file=massif.out ./test.out
	ms_print massif.out

clean:
	rm -f *.out *.o *.a
