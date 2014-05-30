#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/andres/cs/zmq/lib
#error while loading shared libraries: libzmq.so.3: cannot open shared object file: No such file or directory
#CXX=g++-4.7 -std=c++11
CXX=g++ -std=c++11
ZMQ=/home/andres/cs/zmq
ZMQ_INCLUDES=$(ZMQ)/include
ZMQ_LIBS=$(ZMQ)/lib

all: torrent tracker

torrent: torrent.o
	$(CXX) -L$(ZMQ_LIBS) -o torrent torrent.o -lzmq -lczmq

torrent.o: torrent.cc
	$(CXX) -I$(ZMQ_INCLUDES) -c torrent.cc

tracker: tracker.o
	$(CXX) -L$(ZMQ_LIBS) -o tracker tracker.o -lzmq -lczmq

tracker.o: tracker.cc
	$(CXX) -I$(ZMQ_INCLUDES) -c tracker.cc

clean:
	rm -f torrent torrent.o tracker tracker.o *.torrent ASOIAF/*
	rmdir ASOIAF