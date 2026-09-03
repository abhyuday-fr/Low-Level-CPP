#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <vector>

#define MAX_EVENTS 1024
#define MAX_IN_FLIGHT 500

/*
 * The STL unordered_map isn't much nice for the scanner.
 * Considering some DoD, we keep the data structures intrusive and memory
 * friendly
 */

// timeout threshold, 2000 milliseconds for example
const auto TIMEOUT_MS = std::chrono::milliseconds(2000);

struct Connection {
  bool active = false;
  int port = -1;
  std::chrono::steady_clock::time_point start_time;

  // intrusive doubly-linked list pointers
  // instead of pointers, used indeces as raw pointers are invalidated on
  // reallocation, -1 means no neighbour
  int next = -1;
  int prev = -1;
};

struct ConnList { // the intrusive list to be embedded
  int head = -1;  // -1 means no neighbour
  int tail = -1;
};

static void fd_set_nb(int fd) {
  errno = 0;
  int flags = fcntl(fd, F_GETFL, 0);
  if (errno) {
    perror("fcntl error");
    return;
  }

  flags |= O_NONBLOCK;

  errno = 0;
  (void)fcntl(fd, F_SETFL, flags);
  if (errno) {
    perror("fcntl error");
  }
}

// return the max fd value this process could ever be handed.
// helps to know the the size of flat indexed vector
static int get_max_fd_limit() {
  struct rlimit r1;
  if (getrlimit(RLIMIT_NOFILE, &r1) != 0) {
    perror("getrlimit failed");
    return 4096; // fallback to a conservative default if getrlimit somehow
                 // defaults
  }
  return static_cast<int>(r1.rlim_cur); // soft limit
}

// Intrusive List Operations

static void list_insert_tail(std::vector<Connection> &table, ConnList &list,
                             int fd) {
  table[fd].prev = list.tail;
  table[fd].next = -1;

  if (list.tail == -1) { // list was empty
    list.head = fd;
    list.tail = fd;
  } else {
    table[list.tail].next = fd;
    list.tail = fd;
  }
}

static void list_unlink(std::vector<Connection> &table, ConnList &list,
                        int fd) {
  int p = table[fd].prev;
  int n = table[fd].next;

  if (p == -1 && n == -1) { // case 1: only element in the list
    list.head = -1;
    list.tail = -1;
  } else if (p == -1) { // case 2: head but no tail
    list.head = n;
    table[n].prev = -1;
  } else if (n == -1) { // case 3: tail but no head
    list.tail = p;
    table[p].next = -1;
  } else { // case 4: somewhere in the middle
    table[p].next = n;
    table[n].prev = p;
  }
}

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <target-ip> <start-port> <end-port>\n";
    return EXIT_FAILURE;
  }

  const char *target_ip = argv[1];
  int start_port = std::stoi(argv[2]);
  int end_port = std::stoi(argv[3]);

  if (start_port < 1 || end_port > 65535 || start_port > end_port) {
    std::cerr << "Invalid range of ports\nHints:\n1. end-port <= 65535\n2. "
                 "start-port > 1\n3. start-port < end-port\n";
    exit(EXIT_FAILURE);
  }

  std::cout << "Knocking on " << target_ip << " from port " << start_port
            << " to " << end_port << "\n";

  // size the flat indexed table to the kernel's actual fd ceiling
  const int max_fd = get_max_fd_limit();
  std::vector<Connection> table(max_fd);
  ConnList in_flight; // iintrusive list ordered by connect time

  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    perror("epoll_creat1 failed");
    return EXIT_FAILURE;
  }

  int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

  // configure the timer to pulse every 500ms
  struct itimerspec ts;
  ts.it_interval.tv_sec = 0;
  ts.it_interval.tv_nsec = 500000000; // 500 ms interval
  ts.it_value.tv_sec = 0;
  ts.it_value.tv_nsec = 500000000; // first pulse in 500 ms
  timerfd_settime(timer_fd, 0, &ts, NULL);

  struct epoll_event timer_ev;
  timer_ev.events = EPOLLIN;
  timer_ev.data.u64 = ((uint64_t)timer_fd << 32) | 0;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &timer_ev);

  struct epoll_event events[MAX_EVENTS];

  int port = start_port;
  int in_flight_count = 0;

  while (port <= end_port || in_flight_count > 0) {

    // fill the queue upto the concurrency limit
    while (in_flight_count < MAX_IN_FLIGHT && port <= end_port) {

      int sock = ::socket(AF_INET, SOCK_STREAM, 0);
      if (sock < 0) {
        std::cerr << "Couldn't create the socket: " << strerror(errno);
        return EXIT_FAILURE;
      }

      if (sock > max_fd) { // very less chances of happening
        std::cerr << "fd " << sock
                  << " exceeds sized table (max_fd = " << max_fd << ")\n";
        ::close(sock);
        return EXIT_FAILURE;
      }

      fd_set_nb(sock);

      // bind/inet_pton
      sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(port);

      if (::inet_pton(AF_INET, target_ip, &addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address\n";
        ::close(sock);
        return EXIT_FAILURE;
      }

      int res = ::connect(sock, (struct sockaddr *)&addr, sizeof(addr));

      if (res == 0) { // rare but localhost might connect instantly
        std::cout << "Port " << port << " answered the door!\n";
        close(sock);
      } else if (errno == EINPROGRESS) {
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.u64 = ((uint64_t)sock << 32) | (uint32_t)port;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);

        table[sock].active = true;
        table[sock].port = port;
        table[sock].start_time = std::chrono::steady_clock::now();
        list_insert_tail(table, in_flight, sock);
        in_flight_count++;
      } else { // immediate failure (maybe network is unreachable)
        ::close(sock);
      }
      port++;
    }

    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    for (int i = 0; i < n; i++) {
      // unpack our fd and port
      int fd = events[i].data.u64 >> 32;
      int p = events[i].data.u64 & 0xFFFFFFFF;

      if (p == 0) {
        // timer triggered: sweep for timed-out sockets
        // logic: sweep for the head only, stop at first "not yet expired"
        // connection (sockets are already stored in order of their timestamps)
        uint64_t expirations;
        read(timer_fd, &expirations, sizeof(expirations));

        auto now = std::chrono::steady_clock::now();
        while (in_flight.head != -1 &&
               now - table[in_flight.head].start_time > TIMEOUT_MS) {
          int expired_fd = in_flight.head;

          list_unlink(table, in_flight, expired_fd);
          epoll_ctl(epoll_fd, EPOLL_CTL_DEL, expired_fd, NULL);
          close(expired_fd);
          table[expired_fd].active = false;
          in_flight_count--;
        }
      } else {
        // socket ready
        // connected or rejected
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

        if (err == 0) {
          std::cout << "Port " << p << " answered the door!\n";
        }

        // cleanup
        list_unlink(table, in_flight, fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        table[fd].active = false;
        in_flight_count--;
      }
    }
  }

  close(timer_fd);
  close(epoll_fd);
  return EXIT_SUCCESS;
}
