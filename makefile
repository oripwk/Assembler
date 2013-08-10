objects = main.o pass1.o pass2.o hash.o lib.o

asm : $(objects)
	gcc -o asm $(objects) -Wall -g

main.o : main.c main.h pass1.h pass2.h hash.h lib.h
	gcc -c main.c -g

pass1.o : pass1.c main.c pass1.h main.h hash.h lib.h
	gcc -c pass1.c -g

pass2.o : pass2.c main.c pass1.h main.h hash.h lib.h
	gcc -c pass2.c -g

hash.o : hash.c hash.h lib.h
	gcc -c hash.c -g

lib.o : lib.c main.h
	gcc -c lib.c -g

clean :
	rm $(objects)
