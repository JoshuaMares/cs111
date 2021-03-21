default:
	gcc -lm -lmraa -lssl -lcrypto -o lab4c -g -Wall -Wextra lab4c.c
	ln -f lab4c lab4c_tcp
	ln -f lab4c lab4c_tls
	chmod +x lab4c_tcp
	chmod +x lab4c_tls

clean:
	rm -f lab4c-005154394.tar.gz lab4c lab4c_tcp lab4c_tls

dist: default
	tar -czvf lab4c-005154394.tar.gz lab4c.c Makefile README
