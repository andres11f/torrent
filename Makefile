#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/andres/cs/zmq/lib
#error while loading shared libraries: libzmq.so.3: cannot open shared object file: No such file or directory
#CXX=g++-4.7 -std=c++11
CXX=g++ -std=c++11 -pthread
ZMQ=/home/andres/cs/zmq
ZMQ_INCLUDES=$(ZMQ)/include
ZMQ_LIBS=$(ZMQ)/lib

all: torrent tracker

torrent: torrent.o
	$(CXX) -L$(ZMQ_LIBS) -o torrent torrent.o -lzmq -lczmq
	mkdir torrent1 && cp torrent torrent1/
	mkdir torrent2 && cp torrent torrent2/
	mkdir torrent3 && cp torrent torrent3/

torrent.o: torrent.cc
	$(CXX) -W -Wall -Werror -I$(ZMQ_INCLUDES) -c torrent.cc

tracker: tracker.o
	$(CXX) -L$(ZMQ_LIBS) -o tracker tracker.o -lzmq -lczmq

tracker.o: tracker.cc
	$(CXX) -W -Wall -Werror -I$(ZMQ_INCLUDES) -c tracker.cc

clean:
	rm -f torrent torrent.o tracker tracker.o *.torrent ASOIAF_download.txt
	rm -r torrent1 torrent2 torrent3