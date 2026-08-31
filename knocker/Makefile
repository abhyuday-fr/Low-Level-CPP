CXX = g++

all: knocker knocker-epoll knocker-epoll-timer

knocker:
	$(CXX) knocker.cc -o knocker

knocker-epoll:
	$(CXX) knocker-epoll.cc -o knocker-epoll

knocker-epoll-timer:
	$(CXX) knocker-epoll-timer.cc -o knocker-epoll-timer

clean:
	rm -rf knocker knocker-epoll knocker-epoll-timer

.PHONY: all clean
