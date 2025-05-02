OPTIONS=--std=c99 -Wall -g

flashsim: main.o disk.o flash.o
	gcc main.o disk.o flash.o -o flashsim

main.o: main.c disk.h flash.h
	gcc ${OPTIONS} -c main.c -o main.o

disk.o: test.c disk.h
	gcc ${OPTIONS} -c test.c -o disk.o

flash.o: flash.c flash.h
	gcc ${OPTIONS} -c flash.c -o flash.o

clean:
	rm -f flashsim *.o

