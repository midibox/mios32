CC=g++

minfs_test: minfs_test.o minfs.o minfs_ram.o
	g++ minfs_test.o minfs.o minfs_ram.o -o minfs_test

minfs_test.o: minfs_test.c
	g++ minfs_test.c -o minfs_test.o -c

minfs.o: ../minfs.c
	g++ ../minfs.c -o minfs.o -c
	
minfs_ram.o: ../minfs_ram.c 
	g++ ../minfs_ram.c -o minfs_ram.o -c


clean:
	rm -rf *.o
	