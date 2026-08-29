#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * to integrate epoll, we need to shift from a linear "create and wait" mindset
 * to an event driven architecture
 * Instead of waiting for a connection to finish before moving to the next port,
 * we will fire off hundreds of non-blocking connection attempts and let the
 * kernel notify us when they resolve.
 */

// system's file descriptor limit (usually 1024 by default on Linux)
#define MAX_EVENTS 1024

// concurrency limit, prevents crashing the ulimit -n
#define MAX_IN_FLIGHT 500

/*
 * When using port scanner in an external site (you should have the permission
 * to do so) The site's fireweall might drop the unexpected packets instead of
 * rejecting them. As a result, the 500 sockets will fill up immediately and the
 * scanner will hang for responses that are never coming.
 * That's why we need to implement a custom timer handling using timerfd
 */

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

  struct epoll_event events[MAX_EVENTS];
  int in_flight = 0;

  // loop through the specified port range
  for (int port = start_port; port <= end_port; port++) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      std::cerr << "Couldn't create the socket: " << strerror(errno);
      return EXIT_FAILURE;
    }

    // bind/inet_pton
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    if (::inet_pton(AF_INET, target_ip, &server_address.sin_addr) <= 0) {
      std::cerr << "Invalid IP address\n";
      ::close(sock);
      return EXIT_FAILURE;
    }

    // set the listening fd to nonblocking mode before connecting
    fd_set_nb(sock);

    // initiate connection
    int res = ::connect(sock, (struct sockaddr *)&server_address,
                        sizeof(server_address));

    if (res == 0) { // rare but localhost might connect instantly
      std::cout << "Port " << port << " answered the door!\n";
      close(sock);
    } else if (errno == EINPROGRESS) {
      // connection is pending.. register with epoll
      struct epoll_event ev;
      ev.events = EPOLLOUT;

      // Low-Level Trick: pack both fd and port in the 64-bit user data payload
      // so we don't need a separate hashmap to track which fd belongs to which
      // port
      ev.data.u64 = ((uint64_t)sock << 32) | (uint32_t)port;
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);
      in_flight++;
    } else { // immediate failure (maybe network is unreachable)
      ::close(sock);
    }

    // If we hit the concurrency limit, or we are on the final port
    // block and process the epoll event queue
    while (in_flight >= MAX_IN_FLIGHT || (port == end_port && in_flight > 0)) {
      int n =
          epoll_wait(epoll_fd, events, MAX_EVENTS, -1); // -1 is indefinitely
      // the epoll_wait populates the events array we made earlier with the
      // events(ev)

      for (int i = 0; i < n; i++) {
        // unpack our fd and port
        int fd = events[i].data.u64 >> 32;
        int p = events[i].data.u64 & 0xFFFFFFFF;

        // verify if the connection actually succeeded
        int err = 0;
        socklen_t len = sizeof(err);
        getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &len);

        if (err == 0) {
          std::cout << "Port " << p << " answered the door!\n";
        }

        // cleanup
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        in_flight--;
      }
    }
  }

  close(epoll_fd);
  return EXIT_SUCCESS;
}
