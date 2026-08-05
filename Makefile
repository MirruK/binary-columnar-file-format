CC := clang

test-lib: sized_bincoff_buffer.o test_sized_bincoff_buffer.o
	$(CC) -g sized_bincoff_buffer.o test_sized_bincoff_buffer.o -o test-sized_bincoff_buffer -Lvendor/criterion-2.4.3/lib/ -Ivendor/criterion-2.4.3/include/ -lcriterion -Wl,-rpath,'$$ORIGIN/vendor/criterion-2.4.3/lib'


# Embeds link path of criterion into executable, meaning it will find the dynamically linked vendored dependency
test: bincoff.o sized_bincoff_buffer.o tests.o
	$(CC) -g sized_bincoff_buffer.o bincoff.o tests.o -o test-bincoff -Lvendor/criterion-2.4.3/lib/ -Ivendor/criterion-2.4.3/include/ -lcriterion -Wl,-rpath,'$$ORIGIN/vendor/criterion-2.4.3/lib'

test_sized_bincoff_buffer.o: test_sized_bincoff_buffer.c
	$(CC) -g -c -Ivendor/criterion-2.4.3/include test_sized_bincoff_buffer.c -o test_sized_bincoff_buffer.o

tests.o: tests.c
	$(CC) -g -c -Ivendor/criterion-2.4.3/include tests.c -o tests.o

sized_bincoff_buffer.o: sized_bincoff_buffer.c
	$(CC) -g -c sized_bincoff_buffer.c -o sized_bincoff_buffer.o

bincoff.o: bincoff.c
	$(CC) -g -c bincoff.c -o bincoff.o

debug.o: debug.c
	$(CC) -g -Og -fno-eliminate-unused-debug-symbols -fsanitize=address -Wall -Wextra -c debug.c -o debug.o

main.o: main.c
	$(CC) -g -c main.c -o main.o

cli: bincoff.o sized_bincoff_buffer.o main.o
	$(CC) bincoff.o sized_bincoff_buffer.o main.o -o bincoff-cli

debug: bincoff.o debug.o sized_bincoff_buffer.o
	$(CC) -g -Og -lasan bincoff.o debug.o sized_bincoff_buffer.o -o debug-bincoff

