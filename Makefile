
all: lab2_list

lab2_list: lab2_list.c
	gcc -Wall -Wextra -g lab2_list.c SortedList.c -o lab2_list -lpthread -lm


tests: 
	> lab2b_list.csv
	for its in 1000 ; do \
		for thr in 1 2 4 8 12 16 24 ; do \
			./lab2_list --threads $$thr --iterations $$its --sync=m ; \
			./lab2_list --threads $$thr --iterations $$its --sync=s ; \
		done \
	done	

	-for its in 1 2 4 8 16 ; do \
		for thr in 1 4 8 12 16; do \
			./lab2_list  --lists 4 --threads $$thr --iterations $$its --yield=id ; \
		done \
	done

	for its in 10 20 40 80 ; do \
		for thr in 1 4 8 12 16; do \
			./lab2_list  --lists 4 --threads $$thr --iterations $$its --yield=id  --sync=m; \
			./lab2_list  --lists 4 --threads $$thr --iterations $$its --yield=id  --sync=s; \
		done \
	done

	for list_var in  4 8 16; do \
		for thr in 1 2 4 8 12; do \
			./lab2_list  --lists $$list_var --threads $$thr --iterations 1000 --sync=m; \
			./lab2_list  --lists $$list_var --threads $$thr --iterations 1000 --sync=s; \
		done \
	done

graphs: lab2_list.csv
	gnuplot lab2_list.gp

profile.out: 
	rm -f perf.data perf.data.old 
	perf record ./lab2_list --threads=12 --iterations=1000 --sync=s
	perf report > $@
	perf annotate >> $@
	rm -f perf.data

profile: profile.out


clean:
	rm -f lab2_list  lab2b-40205638.tar.gz  *.data *.data.old 

dist:  lab2_list profile tests
	tar -zcvf lab2b-40205638.tar.gz README Makefile *.c *.h *.csv *.png  lab2_list.gp lab2b_list.csv profile.out

clobber:
	rm -f lab2_list  lab2a-40205638.tar.gz *.csv *.png