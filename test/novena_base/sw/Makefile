all: hash_tester

.c.o:
	gcc -c -Wall -o $@ $<

hash_tester : hash_tester.o novena-eim.o
	gcc -o hash_tester hash_tester.o novena-eim.o

hash_tester.o: hash_tester.c novena-eim.h

novena-eim.o: novena-eim.c novena-eim.h

clean:
	rm -f *.o hash_tester
