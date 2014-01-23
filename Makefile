client: client.o 
	gcc client.o -o client

client.o: client.c
	gcc -Wall -c client.c

clean:
	rm *.o
	rm client
