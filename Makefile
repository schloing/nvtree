CC_FLAGS = -Wall -O2
OBJ = main.o

.PHONY: clean run debug massif

libnvtree.a: $(OBJ)
	ar rcs $@ $^

main.o: main.c nvtree.h
	gcc $(CC_FLAGS) -c main.c -o main.o

test: test/main.c libnvtree.a
	gcc $(CC_FLAGS) $^ -L. -lnvtree -o test.out
	./test.out

massif: test
	valgrind --tool=massif --massif-out-file=massif.out ./test.out
	ms_print massif.out

clean:
	rm -f *.out *.o *.a
