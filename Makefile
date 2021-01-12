default: lab.c
	gcc -o lab0 -g -Wall -Wextra lab0.c

check:

clean:
	rm -f lab0 lab0.tar.gz

dist: default
	tar -czvf lab0.tar.gz lab0.c Makefile backtrace.png breakpoint.png README
