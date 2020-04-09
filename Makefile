webproxy: webproxy.o threads.o helpers.o
	gcc -pthread $^ -o $@

threads.o: threads.c threads.h 
	gcc -c threads.c

helpers.o: helpers.c helpers.c
	gcc -c helpers.c
	
webproxy.o: webproxy.c webproxy.h
	gcc -c webproxy.c

clean:
	rm *.o
	rm webproxy
