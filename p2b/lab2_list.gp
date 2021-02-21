#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
# 8. lock wait time (ns)
#
# output:
#	lab2_list-1.png ... Total Operations/Second for each Synchronization Method vs Threads
#	lab2_list-2.png ... Wait-For-Lock Time, and Average Time Per Operation vs Threads
#	lab2_list-3.png ... Threads and Iterations that run without failure
#	lab2_list-4.png ... Aggregated Throughput vs Threads - Mutex Lock
#	lab2_list-5.png ... Aggregated Throughput vs Threads - Spin Lock
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

set title "List-1: Total Operations/Second for each Synchronization Method vs Threads"
set xlabel "Threads"
set xrange [0.75:32]
set logscale x 2
set ylabel "Operations/Second (ops/s)"
set logscale y 10
set output 'lab2b_1.png'

# syncs, 1-24 threads,1000 iterations, 1 list, random
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'mutex lock' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'spin lock ' with linespoints lc rgb 'green'


set title "List-2: Wait-For-Lock Time, and Average Time Per Operation vs Threads"
set xlabel "Threads"
set xrange [0.75:32]
set logscale x 2
set ylabel "TIme per Operation (ns)"
set logscale y 10
set output 'lab2b_2.png'

#png 2
plot \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
	title 'avg time per op' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($8) \
	title 'mutex lock wait time' with linespoints lc rgb 'green'


set title "List-3: Threads and Iterations that run without failure"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep list-id-none, lab2b_list.csv" using ($2):($3) \
	title 'yield=id' with points lc rgb 'green', \
     "< grep list-id-m, lab2b_list.csv" using ($2):($3) \
	title 'yield=id, mutex' with points lc rgb 'red', \
     "< grep list-id-s, lab2b_list.csv" using ($2):($3) \
	title 'yield=id, spin' with points lc rgb 'violet'


set title "List-4: Aggregated Throughput vs Threads - Mutex Lock"
set xlabel "Threads"
set xrange [0.75:32]
set logscale x 2
set ylabel "Operations/Second (ops/s)"
set logscale y 10
set output 'lab2b_4.png'

#png 4
plot \
    "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title '1 list' with linespoints lc rgb 'red', \
    "< grep 'list-none-m,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title '4 lists' with linespoints lc rgb 'green', \
    "< grep 'list-none-m,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title '8 lists' with linespoints lc rgb 'blue', \
    "< grep 'list-none-m,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title '16 lists' with linespoints lc rgb 'orange'


set title "List-5: Aggregated Throughput vs Threads - Spin Lock"
set xlabel "Threads"
set xrange [0.75:32]
set logscale x 2
set ylabel "Operations/Second (ops/s)"
set logscale y 10
set output 'lab2b_5.png'

#png 5 - names,threads,iterations,lists
plot \
    "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title '1 list' with linespoints lc rgb 'red', \
    "< grep 'list-none-s,[0-9]*,1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title '4 lists' with linespoints lc rgb 'green', \
    "< grep 'list-none-s,[0-9]*,1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title '8 lists' with linespoints lc rgb 'blue', \
    "< grep 'list-none-s,[0-9]*,1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
  title '16 lists' with linespoints lc rgb 'orange'
