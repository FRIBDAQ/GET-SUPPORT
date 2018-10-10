statechange.h: statechange.ggo
	gengetopt -Fstatechange <statechange.ggo
	gcc -c statechange.c

statechange.o: statechange.ggo
	gengetopt -Fstatechange <statechange.ggo
	gcc -c statechange.c

clean:
	rm -f *.o statechange.h statechange.c

