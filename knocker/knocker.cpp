#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

// TODO : make this multithreaded (preferrably with a thread pool)

int main(int argc, char *argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0]
              << " <target-ip> <start-port> <end-port>\n";
    return EXIT_FAILURE;
  }

  const char *target_ip = argv[1];
  int start_port = std::atoi(argv[2]);
  int end_port = std::atoi(argv[3]);

  std::cout << "Knocking on " << target_ip << " from port " << start_port
            << " to " << end_port << "\n";

  // loop through the specified port range
  for (int port = start_port; port <= end_port; port++) {
    int sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
      std::cerr << "Could not create socket: " << strerror(errno);
      return EXIT_FAILURE;
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);

    // convert IP address from text to binary form
    if (::inet_pton(AF_INET, target_ip, &server_address.sin_addr) <= 0) {
      std::cerr << "Invalid IP address\n";
      ::close(sock);
      return EXIT_FAILURE;
    }

    // Attempt to connect to the port
    if (::connect(sock, (struct sockaddr *)&server_address,
                  sizeof(server_address)) == 0) {
      std::cout << "Port " << port << " answered the door!\n";
    }
    close(sock);
  }
}
