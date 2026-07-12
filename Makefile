test: bincoff.o tests.o
	# Embeds link path of criterion into executable, meaning it will find the dynamically linked vendored dependency
	gcc bincoff.o tests.o -o test-bincoff -Lvendor/criterion-2.4.3/lib/ -Ivendor/criterion-2.4.3/include/ -lcriterion -Wl,-rpath,'$$ORIGIN/vendor/criterion-2.4.3/lib'
	
tests.o: tests.c
	gcc -c -Ivendor/criterion-2.4.3/include tests.c -o tests.o

bincoff.o: bincoff.c
	gcc -c bincoff.c -o bincoff.o

main.o: main.c
	gcc -g -c main.c -o main.o

cli: bincoff.o main.o
	gcc bincoff.o main.o -o bincoff-cli
