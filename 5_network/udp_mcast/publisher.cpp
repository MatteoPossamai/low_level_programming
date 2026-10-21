#include "common.hpp"
#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
  int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (!udp_sock) {
    perror("Socker");
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(SERVER_PORT);
  addr.sin_addr.s_addr = inet_addr(MULTICAST_ADDR);

  size_t counter = 0;

  while (true) {
    std::string message =
        std::to_string(counter) + "|Message " + std::to_string(counter);
    ++counter;
    ssize_t res = sendto(udp_sock, message.data(), message.size(), 0,
                         (struct sockaddr *)&addr, sizeof(addr));
    if (res < 0) {
      perror("Sending message");
      return -1;
    }

    std::cout << "Sent: " << message << std::endl;
    sleep(1);
  }

  return 0;
}
