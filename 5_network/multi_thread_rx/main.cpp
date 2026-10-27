// Make with epoll a receiver that can handle and put together
// multiple receiver streams.

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

constexpr size_t PORT = 8080;
constexpr size_t SENDERS = 2;
constexpr size_t MESSAGE_SIZE = 10;

void sender_f() {
  int fds_[SENDERS];

  for (size_t i = 0; i < SENDERS; i++) {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    if (connect(clientSocket, (struct sockaddr *)&serverAddress,
                sizeof(serverAddress)) < 0) {
      perror("Connect");
    }
    fds_[i] = clientSocket;
  }

  for (size_t i = 0; i < SENDERS; i++) {
    std::string part1 = ">>msg";
    send(fds_[i], part1.data(), part1.size(), 0);
  }
  for (size_t i = 0; i < SENDERS; i++) {
    std::string part2 = "end<<<<";
    send(fds_[i], part2.data(), part2.size(), 0);
  }
}

int main() {
  std::unordered_set<int> fds;
  std::unordered_map<int, std::string> buffers;
  int rx_fd = socket(AF_INET, SOCK_STREAM, 0);
  int reuse_addr = 1;
  if (setsockopt(rx_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
                 sizeof(reuse_addr)) < 0) {
    perror("setsockopt SO_REUSEADDR");
    close(rx_fd);
    return 1;
  }
  fds.insert(rx_fd);
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(PORT);
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  if (bind(rx_fd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) <
          0 ||
      listen(rx_fd, 10) < 0) {
    perror("bind");
    return 1;
  }

  // EPOLL setup
  int epfd = epoll_create1(0);
  if (epfd < 0) {
    perror("epoll_create1");
    return 1;
  }

  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = rx_fd;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, rx_fd, &ev) < 0) {
    perror("epoll_ctl");
    return 1;
  }

  std::jthread t(sender_f);
  struct epoll_event events[10];
  size_t printed = 0;
  while (printed < 2) {
    int n = epoll_wait(epfd, events, 10, -1);

    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;
      if (!(events[i].events & EPOLLIN))
        continue;

      if (fds.contains(fd)) {
        // File descriptor of new client
        int client_fd = accept4(fd, nullptr, nullptr, SOCK_NONBLOCK);
        if (client_fd < 0) {
          perror("accept4");
          continue;
        }

        struct epoll_event cev;
        cev.events = EPOLLIN;
        cev.data.fd = client_fd;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &cev) < 0) {
          perror("epoll_ctl client");
          continue;
        }
        buffers[client_fd] = "";
      } else {
        std::string buf(MESSAGE_SIZE, '\0');

        ssize_t n_in =
            read(fd, buf.data(), MESSAGE_SIZE - buffers.at(fd).size());

        if (n_in > 0) {
          buf.resize(n_in);
          buffers.at(fd) += buf;
        }
        if (buffers.at(fd).size() >= MESSAGE_SIZE) {
          std::cout << "" << "SENDER " << fd << ":" << buffers.at(fd)
                    << std::endl;
          buffers.at(fd) = "";
          printed++;
        }
      }
    }
  }

  return 0;
}
