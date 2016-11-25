all: transmitter receiver getip

transmitter:
	g++ -std=c++11 -o bin/transmitter src/transmitter.cpp -lpthread

receiver:
	g++ -std=c++11 -o bin/receiver src/receiver.cpp -lpthread

getip:
	g++ -std=c++11 -o bin/getip src/getip.cpp -lpthread

clean:
	rm -f getip receiver transmitter
