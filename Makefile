all:
	g++ -Wall -c -I/usr/include/jsoncpp main.cc UAVGoods.cc
	g++ -o main.exe main.o UAVGoods.o -L/usr/lib/x86_64-linux-gnu -ljsoncpp
