servidor: servidor.o
	g++ -pthread -o servidor servidor.o
servidor.o: servidor.cpp
	g++ -pthread -c servidor.cpp
clean:
	rm -r servidor.o
	rm -r servidor

