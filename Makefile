CC := clang


test: bincoff.o tests.o
	# Embeds link path of criterion into executable, meaning it will find the dynamically linked vendored dependency
	$(CC) -g bincoff.o tests.o -o test-bincoff -Lvendor/criterion-2.4.3/lib/ -Ivendor/criterion-2.4.3/include/ -lcriterion -Wl,-rpath,'$$ORIGIN/vendor/criterion-2.4.3/lib'
	
tests.o: tests.c
	$(CC) -g -c -Ivendor/criterion-2.4.3/include tests.c -o tests.o

bincoff.o: bincoff.c
	$(CC) -g -c bincoff.c -o bincoff.o

debug.o: debug.c
	$(CC) -g -Og -fno-eliminate-unused-debug-symbols -fsanitize=address -Wall -Wextra -c debug.c -o debug.o
	
main.o: main.c
	$(CC) -g -c main.c -o main.o

cli: bincoff.o main.o
	$(CC) bincoff.o main.o -o bincoff-cli

debug: bincoff.o debug.o
	$(CC) -g -lasan bincoff.o debug.o -o debug-bincoff 
