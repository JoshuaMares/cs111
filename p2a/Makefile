default: lab0.c
	gcc -pthread -o lab2_add -g -Wall -Wextra lab2_add.c

check:
	bash ./check.sh

clean:
	rm -f lab0 lab0-005154394.tar.gz

dist:
	tar -czvf lab0-005154394.tar.gz lab0.c Makefile backtrace.png breakpoint.png README check.sh
