all: scsb
	gcc -O3 -o scsb scsb.c

clean:
	rm -f scsb
