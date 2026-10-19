// Protocol
// <message_len>|<message>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

constexpr uint16_t PORT = 8080;
constexpr int ADDRESS_FAMILY = AF_INET;
constexpr size_t MAX_CONN = 5;
constexpr size_t BUFFER_SIZE = 1024;

int main() {
  int sock_fd = socket(ADDRESS_FAMILY, SOCK_STREAM, 0);

  sockaddr_in address;
  address.sin_family = ADDRESS_FAMILY;
  address.sin_port = htons(PORT);
  address.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock_fd, (struct sockaddr *)&address, sizeof(sockaddr_in)) != 0) {
    perror("Bind: ");
    return -1;
  }

  if (listen(sock_fd, MAX_CONN) != 0) {
    perror("Listen:");
    return -1;
  }
  std::cout << "Server listening on port " << PORT << std::endl;

  char buffer[BUFFER_SIZE] = {0};
  bool stopped = false;

  while (!stopped) {
    int client_sock = accept(sock_fd, nullptr, 0);
    bool getting_len = true;
    size_t acc = 0;
    char msg_buffer[BUFFER_SIZE];
    int msg_idx = 0;

    while (!stopped) {
      memset(buffer, 0, BUFFER_SIZE);
      auto size = recv(client_sock, buffer, BUFFER_SIZE, 0);
      std::cout << "Buffer: " << buffer << std::endl;

      int i = 0;

      if (size == 0) {
        stopped = true;
      } else {
        for (; i < size; i++) {
          if (getting_len) {
            if (buffer[i] == '|') {
              getting_len = false;
            } else {
              ;
              acc = acc * 10 + (buffer[i] - '0');
            }
          } else {
            msg_buffer[msg_idx++] = buffer[i];
            acc--;
            if (acc == 0) {
              getting_len = true;
              msg_buffer[msg_idx] = '\0';
              std::cout << msg_buffer << std::endl;
              msg_idx = 0;
              memset(msg_buffer, 0, BUFFER_SIZE);
            }
          }
        }
      }
    }
    close(client_sock);
  }

  close(sock_fd);
  return 0;
}
