default: lab0.c
	gcc -o lab0 -g -Wall -Wextra lab0.c

check:
	bash ./check.sh
	
clean:
	rm -f lab0 lab0-005154394.tar.gz

dist:
	tar -czvf lab0-005154394.tar.gz lab0.c Makefile backtrace.png breakpoint.png README check.sh
