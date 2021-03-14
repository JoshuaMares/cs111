default:
	gcc -lm -lmraa -o lab4b -g -Wall -Wextra lab4b.c

check:
	bash ./check.sh

clean:
	rm -f lab4b lab4b-005154394.tar.gz

dist: default
	tar -czvf lab4b-005154394.tar.gz lab4b.c Makefile README check.sh
