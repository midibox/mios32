CC=gcc

all: minfs_test
minfs_test: minfs_test.o minfs.o minfs_ram.o
	gcc minfs_test.o minfs.o minfs_ram.o -o minfs_test -g

minfs_test.o: minfs_test.c
	gcc minfs_test.c -o minfs_test.o -c -g

minfs.o: ../minfs.c
	gcc ../minfs.c -o minfs.o -c -g
	
minfs_ram.o: ../minfs_ram.c 
	gcc ../minfs_ram.c -o minfs_ram.o -c -g


clean:
	rm -rf *.o
	