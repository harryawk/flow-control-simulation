all: transmitter receiver getip support

transmitter:
	g++ -std=c++11 -o bin/transmitter src/transmitter.cpp -lpthread

receiver:
	g++ -std=c++11 -o bin/receiver src/receiver.cpp -lpthread
	
getip:
	g++ -std=c++11 -o bin/getip src/getip.cpp -lpthread
	
support:
	g++ -std=c++11 -o bin/support src/support.cpp -lpthread

clean:
	rm -f support getip receiver transmitter
