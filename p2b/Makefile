default:
	gcc -pthread -o lab2_list -g -Wall -Wextra lab2_list.c SortedList.c

tests:
	-bash ./test.sh

graphs:
	gnuplot lab2_list.gp

profile:
	LD_PRELOAD=/usr/lib64/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf >> profile.out
	pprof --list=thread_worker ./lab2_list ./raw.gperf >> profile.out

dist: default tests graphs
	tar -czvf lab2b-005154394.tar.gz SortedList.h SortedList.c lab2_list.c lab2b_list.csv lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png test.sh README Makefile lab2_list.gp

clean:
	rm -f lab2b-005154394.tar.gz lab2_list

realclean:
	rm -f lab2b-005154394.tar.gz lab2_list lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png
