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
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <unordered_map>

#define MAX_EVENTS 1024

#define MAX_IN_FLIGHT 500

/*
 * When using port scanner in an external site (you should have the permission
 * to do so) The site's fireweall might drop the unexpected packets instead of
 * rejecting them. As a result, the 500 sockets will fill up immediately and the
 * scanner will hang for responses that are never coming.
 * That's why we need to implement a custom timer handling using timerfd
 */

// timeout threshold, 2000 milliseconds for example
const auto TIMEOUT_MS = std::chrono::milliseconds(2000);

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

  // create the epoll instance
  int epoll_fd = epoll_create1(0);
  if (epoll_fd < 0) {
    perror("epoll_creat1 failed");
    return EXIT_FAILURE;
  }

  // create non-blocking timerfd
  int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);

  // configure the timer to pulse every 500ms
  struct itimerspec ts;
  ts.it_interval.tv_sec = 0;
  ts.it_interval.tv_nsec = 500000000; // 500 ms interval
  ts.it_value.tv_sec = 0;
  ts.it_value.tv_nsec = 500000000; // first pulse in 500 ms
  timerfd_settime(timer_fd, 0, &ts, NULL);

  // register the timerfd with epoll
  // port 0 identifies the timer
  struct epoll_event timer_ev;
  timer_ev.events = EPOLLIN;
  timer_ev.data.u64 = ((uint64_t)timer_fd << 32) | 0;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &timer_ev);

  struct epoll_event events[MAX_EVENTS];

  // track start times for in-flight sockets
  std::unordered_map<int, std::chrono::steady_clock::time_point> active_socks;
  int port = start_port;

  while (port <= end_port || !active_socks.empty()) {

    // fill the queue upto the concurrency limit
    while (active_socks.size() < MAX_IN_FLIGHT && port <= end_port) {

      int sock = ::socket(AF_INET, SOCK_STREAM, 0);
      if (sock < 0) {
        std::cerr << "Couldn't create the socket: " << strerror(errno);
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

      // initiate connection
      int res = ::connect(sock, (struct sockaddr *)&addr, sizeof(addr));

      if (res == 0) { // rare but localhost might connect instantly
        std::cout << "Port " << port << " answered the door!\n";
        close(sock);
      } else if (errno == EINPROGRESS) {
        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.u64 = ((uint64_t)sock << 32) | (uint32_t)port;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);

        // record the start time
        active_socks[sock] = std::chrono::steady_clock::now();
      } else { // immediate failure (maybe network is unreachable)
        ::close(sock);
      }
      port++;
    }

    // wait for events (blocks until socket readiness OR timer pulse)

    int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

    for (int i = 0; i < n; i++) {
      // unpack our fd and port
      int fd = events[i].data.u64 >> 32;
      int p = events[i].data.u64 & 0xFFFFFFFF;

      if (p == 0) {
        // timer triggered: sweep for timed-out sockets
        uint64_t expirations;
        read(timer_fd, &expirations, sizeof(expirations)); // clear the timer

        auto now = std::chrono::steady_clock::now();
        for (auto it = active_socks.begin(); it != active_socks.end();) {
          if (now - it->second > TIMEOUT_MS) {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it->first, NULL);
            close(it->first);
            it = active_socks.erase(it); // safe erasure during iteration
          } else {
            ++it;
          }
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
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        active_socks.erase(fd);
      }
    }
  }

  close(timer_fd);
  close(epoll_fd);
  return EXIT_SUCCESS;
}
