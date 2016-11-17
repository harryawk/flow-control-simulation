all: transmitter receiver

transmitter:
	g++ -std=c++11 -o bin/transmitter src/transmitter.cpp -lpthread

receiver:
	g++ -std=c++11 -o bin/receiver src/receiver.cpp -lpthread

clean:
	rm -f receiver transmitter
