all: streebog_tester_eim

.c.o:
	gcc -c -Wall -o $@ $<

streebog_tester_eim: streebog_tester_eim.o novena-eim.o
	gcc -o $@ $^

streebog_tester_eim.o: streebog_tester_eim.c novena-eim.h

novena-eim.o: novena-eim.c novena-eim.h

clean:
	rm -f *.o streebog_tester_eim
