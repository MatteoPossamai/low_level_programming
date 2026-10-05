#pragma once

#include "messages.hpp"
#include "queues.hpp"
#include <cstddef>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_set>

#define START_PORT_RANGE 49152
#define END_PORT_RANGE 49155
#define MAX_EVENTS_PER_ITER 10

constexpr std::size_t kMaxReqSize =
    std::max(EnterRequestView::WIRE_SIZE, CancelRequestView::WIRE_SIZE);

constexpr std::size_t kMaxResSize =
    std::max({AcceptResponseView::WIRE_SIZE, CancelledResponseView::WIRE_SIZE,
              ExecutedResponseView::WIRE_SIZE});

class NetworkDevice {
  virtual InboundMessage receive_msg() = 0;
  virtual size_t send_msg(OutboundMessage, size_t) = 0;
};

template <size_t QUEUE_SIZE> class NetworkDeviceEpoll : public NetworkDevice {
  int epfd = 0;
  std::unordered_set<int> fds;
  struct epoll_event events[MAX_EVENTS_PER_ITER];
  spsc_queue<InboundMessage, QUEUE_SIZE> queue;

public:
  NetworkDeviceEpoll() {
    epfd = epoll_create1(0);
    if (epfd < 0) {
      perror("epoll_create1");
      exit(1);
    }

    for (int i = START_PORT_RANGE; i <= END_PORT_RANGE; i++) {
      // Setup of the sockets on the range of address used for trading

      // 1. Create socket
      int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
      if (listen_fd < 0) {
        perror("socket DEVICE");
        exit(1);
      }
      // 2. DO not allow socket to fail on restart
      int opt = 1;
      setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

      // 3. Bind the port
      struct sockaddr_in addr;
      memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_addr.s_addr = htonl(INADDR_ANY); // all interfaces
      addr.sin_port = htons(i);                 // host-to-network byte order

      if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
      }

      // 4. Listen
      if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("listen");
        exit(1);
      }

      struct epoll_event ev;
      ev.events = EPOLLIN;
      ev.data.fd = listen_fd;

      // Registering socket with epoll
      if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
        perror("epoll_ctl");
        exit(1);
      }

      fds.insert(listen_fd);
    }
  }

  ~NetworkDeviceEpoll() {
    close(epfd);
    for (const auto &elem : fds) {
      close(elem);
    }
  }

  InboundMessage receive_msg() {
    while (queue.empty()) {
      int n = epoll_wait(epfd, events, MAX_EVENTS_PER_ITER, -1);
      if (n < 0) {
        perror("epoll_wait");
        return InboundMessage{};
      }

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
            close(client_fd);
            continue;
          }
        } else {
          // Put message in queue
          InboundMessage msg;
          msg.client_fd = fd;
          msg.account = fd; // For simplicity, assume that FD is always the same
                            // as account
          ssize_t r = read(fd, msg.bytes.data(), msg.bytes.size());
          if (r <= 0) {
            // 0 = peer closed. close() removes the fd from epoll for us.
            close(fd);
          } else {
            queue.enqueue(std::move(msg));
          }
        }
      }
    }
    InboundMessage message_in;
    queue.dequeue(message_in);
    return message_in;
  }

  size_t send_msg(OutboundMessage message, size_t client_fd) {
    size_t size = 0;
    switch (message.bytes.data()[0]) {
    case static_cast<std::byte>('A'):
      size = AcceptResponseView::WIRE_SIZE;
      break;
    case static_cast<std::byte>('C'):
      size = CancelledResponseView::WIRE_SIZE;
      break;
    case static_cast<std::byte>('E'):
      size = ExecutedResponseView::WIRE_SIZE;
      break;
    }
    ssize_t w = write(client_fd, message.bytes.data(), size);
    if (w <= 0) {
      perror("Cannot send back echo");
      return 1;
    }
    return 0;
  }
};

// class NetworkDeviceIOUring : public NetworkDevice {};
