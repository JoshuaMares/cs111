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

dist: default tests graphs profile
	tar -czvf lab2b-005154394.tar.gz SortedList.h SortedList.c lab2_list.c lab2b_list.csv lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png test.sh README Makefile lab2_list.gp profile.out raw.gperf

clean:
	rm -f lab2b-005154394.tar.gz lab2_list

realclean:
	rm -f lab2b-005154394.tar.gz lab2_list lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png
