#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2_list-1.png ... cost per operation vs threads and iterations
#	lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#	lab2_list-4.png ... cost per operation vs number of threads
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


set title "Scalability-1: Throughput of Synchronized Lists"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput (operations/sec)"
set logscale y 10
set output 'lab2b_1.png'
set key left top
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,' lab2_list.csv" using ($2):(1000000000/$7) \
	title 'list ins/lookup/delete w mutex' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,' lab2_list.csv" using ($2):(1000000000/$7) \
	title 'list ins/lookup/delete w spin lock' with linespoints lc rgb 'green'




set title "Scalability-2: Per-operation Times for Mutex-Protected List Operations"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "mean time/operation (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key left top
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,' lab2_list.csv" using ($2):($7) \
	title 'completion time' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-m,[0-9]*,1000,' lab2_list.csv" using ($2):($8) \
	title 'wait for lock' with linespoints lc rgb 'red'