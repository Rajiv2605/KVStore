./clear.sh
g++ -c cache.cpp
g++ -c server.cpp
g++ -c main.cpp
g++ cache.o server.o main.o -o output
./output
