server: main.o server.o cache.o
	g++ main.o server.o cache.o -o server

main.o: main.cpp
	g++ -c main.cpp

server.o: server.cpp server.hpp
	g++ -c server.cpp

cache.o: cache.cpp server.hpp
	g++ -c cache.cpp

clean:
	rm *.o output