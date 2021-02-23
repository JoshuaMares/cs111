default:
	gcc -o lab3a -g -Wall -Wextra lab3a.c

dist: default
	tar -czvf lab3a-005154394.tar.gz lab3a.c Makefile README

clean:
	rm -f lab3a-005154394.tar.gz lab3a

sanity: clean dist
	bash ./P3A_check.sh 005154394
