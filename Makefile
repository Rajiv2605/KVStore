output: server.o cache.o
	g++ server.o cache.o -o output

server.o: server.cpp server.hpp
	g++ -c server.cpp

cache.o: cache.cpp server.hpp
	g++ -c cache.cpp

clean:
	rm *.o output