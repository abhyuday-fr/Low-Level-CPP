#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// Improve this with an Event Loop

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

  // loop through the specified port range
  for (int port = start_port; port <= end_port; port++) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      std::cerr << "Couldn't create the socket: " << strerror(errno);
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
