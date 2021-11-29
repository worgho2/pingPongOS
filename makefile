main:  
	gcc -o run.o ppos-core-aux.c pingpong-scheduler.c libppos_static.a

clean:
	rm -f *.o