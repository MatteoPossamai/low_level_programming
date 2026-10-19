// Protocol
// <message_len>|<message>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr uint16_t PORT = 8080;
constexpr int ADDRESS_FAMILY = AF_INET;

int main() {
  int sock_fd = socket(ADDRESS_FAMILY, SOCK_STREAM, 0);

  int flag = 1;
  setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

  sockaddr_in address;
  address.sin_family = ADDRESS_FAMILY;
  address.sin_port = htons(PORT);
  address.sin_addr.s_addr = INADDR_ANY;

  if (connect(sock_fd, (struct sockaddr *)&address, sizeof(address)) != 0) {
    perror("Connect:");
    return -1;
  }

  int out;
  // TEST 1 - Trivial one
  char message0[] = {"5|hello"};
  out = write(sock_fd, message0, sizeof(message0) - 1);
  std::cout << "Sent " << out << "bytes" << std::endl;

  // TEST 2 - Split into multiple
  char message1[] = {"5|abc"};
  char message2[] = {"de"};
  out = write(sock_fd, message1, sizeof(message1) - 1);
  std::cout << "Sent " << out << "bytes" << std::endl;
  out = write(sock_fd, message2, sizeof(message2) - 1);
  std::cout << "Sent " << out << "bytes" << std::endl;

  // TEST 3 - Multiple in 1
  char message3[] = {"3|abc5|hello10|hellohello"};
  out = write(sock_fd, message3, sizeof(message3));
  std::cout << "Sent " << out << "bytes" << std::endl;

  close(sock_fd);
  return 0;
}
