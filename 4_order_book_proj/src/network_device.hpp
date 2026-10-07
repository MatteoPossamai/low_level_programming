#pragma once

#include "messages.hpp"
#include "queues.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <liburing.h>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#define START_PORT_RANGE 49152
#define END_PORT_RANGE 49161
#define MAX_EVENTS_PER_ITER 10

constexpr std::size_t kMaxReqSize =
    std::max(EnterRequestView::WIRE_SIZE, CancelRequestView::WIRE_SIZE);

constexpr std::size_t kMaxResSize =
    std::max({AcceptResponseView::WIRE_SIZE, CancelledResponseView::WIRE_SIZE,
              ExecutedResponseView::WIRE_SIZE});

class NetworkDevice {
public:
  int setup_listening_socket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      perror("socket");
      exit(1);
    }
    int enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (sockaddr *)&addr, sizeof(addr)) < 0 ||
        listen(sock, 10) < 0) {
      perror("bind/listen");
      exit(1);
    }
    return sock;
  }
  virtual void receive_msg() = 0;
  virtual size_t send_msg(OutboundMessage message) = 0;
};

template <size_t QUEUE_SIZE> class NetworkDeviceEpoll : public NetworkDevice {
  int epfd = 0;
  std::unordered_set<int> fds;
  struct epoll_event events[MAX_EVENTS_PER_ITER];
  spsc_queue<InboundMessage, QUEUE_SIZE> &queue;

public:
  NetworkDeviceEpoll(
      spsc_queue<InboundMessage, QUEUE_SIZE> &incoming_queue_in,
      int first_port = START_PORT_RANGE)
      : queue(incoming_queue_in) {
    epfd = epoll_create1(0);
    if (epfd < 0) {
      perror("epoll_create1");
      exit(1);
    }

    for (int i = first_port; i < first_port + (END_PORT_RANGE - START_PORT_RANGE + 1);
         i++) {
      // Setup of the sockets on the range of address used for trading
      int listen_fd = setup_listening_socket(i);

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

  void receive_msg() {
    int n = epoll_wait(epfd, events, MAX_EVENTS_PER_ITER, -1);
    if (n < 0) {
      perror("epoll_wait");
      return;
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
        msg.account = static_cast<uint32_t>(fd);
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

  size_t send_msg(OutboundMessage message) {
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
    ssize_t w =
        write(static_cast<int>(message.account), message.bytes.data(), size);
    if (w <= 0) {
      perror("Cannot send back echo");
      return 1;
    }
    return 0;
  }
};

template <size_t QUEUE_SIZE> class NetworkDeviceIOUring : public NetworkDevice {
  spsc_queue<InboundMessage, QUEUE_SIZE> &queue;
  io_uring ring;
  std::mutex submission_mutex;

  enum class EventType { Accept, Read, Write };

  struct Request {
    EventType type;
    int client_fd = -1;
    int listen_fd = -1;
    std::vector<char> buf;
  };

  std::unordered_set<Request *> inflight_requests;
  std::vector<int> listening_fds;

  void queue_accept(int listen_fd) {
    auto req = std::make_unique<Request>(
        Request{EventType::Accept, -1, listen_fd, {}});
    io_uring_prep_accept(claim_sqe(std::move(req)), listen_fd, nullptr, nullptr,
                         0);
  }

  io_uring_sqe *claim_sqe(std::unique_ptr<Request> req) {
    io_uring_sqe *sqe = io_uring_get_sqe(&ring); // never null at this depth
    Request *raw_req = req.get();
    io_uring_sqe_set_data(sqe, raw_req);
    inflight_requests.insert(raw_req);
    req.release();
    return sqe;
  }

  void queue_read(int client_fd) {
    auto req = std::make_unique<Request>(Request{
        EventType::Read, client_fd, -1, std::vector<char>(kMaxReqSize)});
    char *buf = req->buf.data();
    io_uring_prep_recv(claim_sqe(std::move(req)), client_fd, buf, kMaxReqSize,
                       0);
  }

  void queue_write(std::unique_ptr<Request> req, size_t len) {
    req->type = EventType::Write;
    int fd = req->client_fd;
    char *buf = req->buf.data();
    io_uring_prep_send(claim_sqe(std::move(req)), fd, buf, len, 0);
  }

public:
  NetworkDeviceIOUring(
      spsc_queue<InboundMessage, QUEUE_SIZE> &incoming_queue_in,
      int first_port = START_PORT_RANGE)
      : queue(incoming_queue_in) {

    const int setup_result = io_uring_queue_init(QUEUE_SIZE, &ring, 0);
    if (setup_result < 0)
      throw std::runtime_error("io_uring_queue_init failed");

    // Set up each Socket to be able to accepts connections
    for (int i = first_port; i < first_port + (END_PORT_RANGE - START_PORT_RANGE + 1);
         i++) {
      int listen_fd = setup_listening_socket(i);
      listening_fds.push_back(listen_fd);
      queue_accept(listen_fd);
    }

    // Submit all events to uring
    io_uring_submit(&ring);
  }
  ~NetworkDeviceIOUring() {
    io_uring_queue_exit(&ring);
    for (Request *req : inflight_requests)
      delete req;
    for (int fd : listening_fds)
      close(fd);
  }

  void receive_msg() {
    io_uring_cqe *cqe; // cqe == completed queue events
    int ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
      return;
    }

    // Pair with send_msg's unlock after submission. The kernel CQE transfers
    // the request back to this thread, but TSan does not model that handoff.
    std::unique_lock<std::mutex> lock(submission_mutex);

    // Get result and consume event
    std::unique_ptr<Request> req(
        static_cast<Request *>(io_uring_cqe_get_data(cqe)));
    inflight_requests.erase(req.get());
    int res = cqe->res;
    io_uring_cqe_seen(&ring, cqe); // Marks event as consumed

    if (res < 0) {
      if (req->type == EventType::Accept) {
        queue_accept(req->listen_fd);
        io_uring_submit(&ring);
      }
    } else {
      switch (req->type) {
      case EventType::Accept: {
        queue_accept(req->listen_fd);
        queue_read(res);
        io_uring_submit(&ring);
      } break;
      case EventType::Read:
        if (res == 0) { // peer closed
          // Skip close - assumption: if user goes away trading is over
          //  close(req->client_fd);
          break;
        }
        InboundMessage msg;
        memcpy(msg.bytes.data(), req->buf.data(), res);
        msg.account = static_cast<uint32_t>(req->client_fd);
        lock.unlock();
        queue.enqueue(std::move(msg));
        lock.lock();
        queue_read(req->client_fd);
        io_uring_submit(&ring);
        break;
      case EventType::Write:
        if (res == 0) {
          // Skip close - assumption: if user goes away trading is over
          // close(req->client_fd);
          break;
        }
        if (static_cast<std::size_t>(res) < req->buf.size()) {
          req->buf.erase(req->buf.begin(), req->buf.begin() + res);
          const std::size_t remaining = req->buf.size();
          queue_write(std::move(req), remaining);
          io_uring_submit(&ring);
        }
        break;
      }
    }
  }

  size_t send_msg(OutboundMessage message) {
    std::size_t size = 0;
    switch (message.bytes[0]) {
    case static_cast<std::byte>('A'):
      size = AcceptResponseView::WIRE_SIZE;
      break;
    case static_cast<std::byte>('C'):
      size = CancelledResponseView::WIRE_SIZE;
      break;
    case static_cast<std::byte>('E'):
      size = ExecutedResponseView::WIRE_SIZE;
      break;
    default:
      return 1;
    }

    const int fd = static_cast<int>(message.account);
    std::lock_guard<std::mutex> lock(submission_mutex);
    auto req = std::make_unique<Request>(
        Request{EventType::Write, fd, -1, std::vector<char>(size)});
    std::memcpy(req->buf.data(), message.bytes.data(), size);

    queue_write(std::move(req), size);
    const int submitted = io_uring_submit(&ring);
    return submitted < 0 ? 1 : 0;
  }
};
