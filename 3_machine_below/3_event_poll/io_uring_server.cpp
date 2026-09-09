// Test with:
// >> nc localhost 8081
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <liburing.h>
#include <memory>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>

// you submit the operation (accept/recv/send) and the kernel posts the
// result when done. No non-blocking fds.

constexpr int PORT = 8081;
constexpr unsigned QUEUE_DEPTH = 256;
constexpr size_t READ_SIZE = 4096;

enum class EventType { Accept, Read, Write };

struct Request {
  EventType type;
  int client_fd = -1;
  std::vector<char> buf;
};

io_uring ring;

// Grab a submission slot and hand the Request's ownership to the ring.
// Returned pointer still needs an io_uring_prep_* call.
io_uring_sqe *claim_sqe(std::unique_ptr<Request> req) {
  io_uring_sqe *sqe = io_uring_get_sqe(&ring); // never null at this depth
  io_uring_sqe_set_data(sqe, req.release());
  return sqe;
}

void queue_accept(int listen_fd) {
  auto req = std::make_unique<Request>(Request{EventType::Accept, -1, {}});
  io_uring_prep_accept(claim_sqe(std::move(req)), listen_fd, nullptr, nullptr,
                       0);
}

void queue_read(int client_fd) {
  auto req = std::make_unique<Request>(
      Request{EventType::Read, client_fd, std::vector<char>(READ_SIZE)});
  char *buf = req->buf.data();
  io_uring_prep_recv(claim_sqe(std::move(req)), client_fd, buf, READ_SIZE, 0);
}

// Reuses the read's Request and buffer: the echo is zero-copy in userspace.
void queue_write(std::unique_ptr<Request> req, size_t len) {
  req->type = EventType::Write;
  int fd = req->client_fd;
  char *buf = req->buf.data();
  io_uring_prep_send(claim_sqe(std::move(req)), fd, buf, len, 0);
}

int setup_listening_socket(int port) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  int enable = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sock, (sockaddr *)&addr, sizeof(addr)) < 0 || listen(sock, 10) < 0) {
    perror("bind/listen");
    exit(1);
  }
  return sock;
}

int main() {
  int listen_fd = setup_listening_socket(PORT);
  io_uring_queue_init(QUEUE_DEPTH, &ring, 0);

  queue_accept(listen_fd);
  io_uring_submit(&ring);
  printf("Listening on port %d\n", PORT);

  while (true) {
    io_uring_cqe *cqe;
    int ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
      fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
      break;
    }

    // Take ownership back from the ring. res is the syscall's return value:
    // new fd for accept, byte count for recv/send, -errno on failure.
    std::unique_ptr<Request> req(
        static_cast<Request *>(io_uring_cqe_get_data(cqe)));
    int res = cqe->res;
    io_uring_cqe_seen(&ring, cqe);

    if (res < 0) {
      fprintf(stderr, "op failed: %s\n", strerror(-res));
      if (req->client_fd >= 0)
        close(req->client_fd);
      if (req->type == EventType::Accept) // keep accepting
        queue_accept(listen_fd);
    } else {
      switch (req->type) {
      case EventType::Accept:
        queue_accept(listen_fd); // an accept completes once: re-arm
        queue_read(res);
        break;
      case EventType::Read:
        if (res == 0) { // peer closed
          close(req->client_fd);
          break;
        }
        printf("FD %d sent %d bytes\n", req->client_fd, res);
        queue_write(std::move(req), (size_t)res);
        break;
      case EventType::Write:
        queue_read(req->client_fd); // echo until peer closes
        break;
      }
    }
    io_uring_submit(&ring); // one syscall flushes everything queued above
  }

  io_uring_queue_exit(&ring);
  return 0;
}
