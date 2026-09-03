CXX = g++
CFLAGS = -Wall -Wextra -Werror -g

all: knocker knocker-epoll knocker-epoll-timer knocker-intrusive

knocker:
	$(CXX) $(CFLAGS) knocker.cc -o $@

knocker-epoll:
	$(CXX) $(CFLAGS) knocker-epoll.cc -o $@

knocker-epoll-timer:
	$(CXX) $(CFLAGS) knocker-epoll-timer.cc -o $@

knocker-intrusive:
	$(CXX) $(CFLAGS) knocker-intrusive.cc -o $@

clean:
	rm -rf knocker knocker-epoll knocker-epoll-timer knocker-intrusive

.PHONY: all clean
