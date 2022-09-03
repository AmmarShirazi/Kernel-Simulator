
output: os-kernel.o
	g++ -o os-kernel process.cpp scheduler.cpp os-kernel.cpp -lpthread
clean:
	rm *.o "os-kernel"