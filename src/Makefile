all: transmitter receiver

transmitter:
	g++ -std=c++11 -o transmitter transmitter.cpp dcomm.h -lpthread

receiver:
	g++ -std=c++11 -o receiver receiver.cpp dcomm.h -lpthread

clean:
	rm -f receiver transmitter
