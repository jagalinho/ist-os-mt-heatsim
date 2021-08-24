CFLAGS= -g -Wall -pedantic
CC=gcc

heatSim_p4: main_p4.o matrix2d.o barrier.o
	$(CC) $(CFLAGS) -o heatSim_p4 main_p4.o matrix2d.o barrier.o -lpthread

heatSim_p3: main_p3.o matrix2d.o barrier.o
	$(CC) $(CFLAGS) -o heatSim_p3 main_p3.o matrix2d.o barrier.o -lpthread
	
heatSim_p1: main_p1.o matrix2d.o leQueue.o mplib4.o
	$(CC) $(CFLAGS) -o heatSim_p1 main_p1.o matrix2d.o leQueue.o mplib4.o -lpthread
	
heatSim_p0: main_p0.o matrix2d.o
	$(CC) $(CFLAGS) -o heatSim_p0 main_p0.o matrix2d.o

main_p0.o: main_p0.c matrix2d.h
	$(CC) $(CFLAGS) -c main_p0.c

main_p1.o: main_p1.c matrix2d.h mplib4.h
	$(CC) $(CFLAGS) -c main_p1.c

main_p3.o: main_p3.c matrix2d.h barrier.h
	$(CC) $(CFLAGS) -c main_p3.c
	
main_p4.o: main_p4.c matrix2d.h barrier.h
	$(CC) $(CFLAGS) -c main_p4.c
	
matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -c matrix2d.c

leQueue.o: leQueue.c leQueue.h
	$(CC) $(CFLAGS) -c leQueue.c
	
mplib4.o: mplib4.c mplib4.h leQueue.h
	$(CC) $(CFLAGS) -c mplib4.c
	
barrier.o: barrier.c barrier.h
	$(CC) $(CFLAGS) -c barrier.c

clean:
	rm -f *.o heatSim_p0 heatSim_p1 heatSim_p3 heatSim_p4
