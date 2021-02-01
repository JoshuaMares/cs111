default: lab1b-client.c lab1b-server.c
	gcc -o lab1b-client -g -Wall -Wextra lab1b-client.c
	gcc -o lab1b-server -g -Wall -Wextra lab1b-server.c

lab1b-client: lab1b-client.c
	gcc -o lab1b-client -g -Wall -Wextra lab1b-client.c

lab1b-server: lab1b-server.c
	gcc -o lab1b-server -g -Wall -Wextra lab1b-server.c

clean:
	rm -f lab1b-client lab1b-server lab1b-005154394.tar.gz

dist:
	tar -czvf lab1b-005154394.tar.gz lab1b-client.c lab1b-server.c Makefile README
