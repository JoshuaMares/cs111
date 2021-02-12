build:
	gcc -pthread -o lab2_add -g -Wall -Wextra lab2_add.c
	gcc -pthread -o lab2_list -g -Wall -Wextra lab2_list.c SortedList.c

tests:
	-bash ./test.sh

graphs:
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

dist: build tests graphs
	tar -czvf lab2a-005154394.tar.gz lab2_add.c SortedList.h SortedList.c lab2_list.c lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png test.sh README Makefile lab2_add.gp lab2_list.gp

clean:
	rm -f lab2a-005154394.tar.gz lab2_add lab2_list

realclean:
	rm -f lab2a-005154394.tar.gz lab2_add lab2_list lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png
