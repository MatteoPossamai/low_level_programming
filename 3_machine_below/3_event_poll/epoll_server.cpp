// Test with:
// >> nc localhost 8080
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

// epoll is a notification system, no async interaction as other
// tools as io_uring or Windows counter part

#define PORT 8080

// Server logic
int main() {
  // Epoll file descriptor
  int epfd = epoll_create1(0);
  if (epfd < 0) {
    perror("epoll_create1");
    return 1;
  }

  // Listening socket, non-blocking so a spurious wakeup can't stall us
  int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (listen_fd < 0) {
    perror("socket");
    return 1;
  }

  // Without this, restarting within ~60s fails with "Address already in use"
  // (kernel keeps the port in TIME_WAIT)
  int opt = 1;
  setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY); // all interfaces
  addr.sin_port = htons(PORT);              // host-to-network byte order

  if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    return 1;
  }
  if (listen(listen_fd, SOMAXCONN) < 0) {
    perror("listen");
    return 1;
  }

  // Event for the listening socket
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = listen_fd;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
    perror("epoll_ctl");
    return 1;
  }

  // this is another epoll_event array to be filled by the kernel
  struct epoll_event events[10];

  printf("Listening on port %d\n", PORT);

  while (1) {
    // Wait for events from the registered fds
    int n = epoll_wait(epfd, events, 10, -1);
    if (n < 0) {
      perror("epoll_wait");
      return 1;
    }

    // Ready FDs
    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;

      // check if the mask is as we wanted
      if (!(events[i].events & EPOLLIN))
        continue;

      if (fd == listen_fd) {
        // EPOLLIN on the listening socket = connection waiting.
        // accept4 sets non-blocking on the new fd atomically.
        int client_fd = accept4(listen_fd, nullptr, nullptr, SOCK_NONBLOCK);
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
        printf("New client on FD %d\n", client_fd);
      } else {
        // EPOLLIN on a client socket = data waiting, read won't block
        char buf[4096];
        ssize_t r = read(fd, buf, sizeof(buf));
        if (r <= 0) {
          // 0 = peer closed. close() removes the fd from epoll for us.
          printf("FD %d closed\n", fd);
          close(fd);
        } else {
          printf("FD %d sent %zd bytes:\n>> ", fd, r);
          for (int idx = 0; idx < 4096 && buf[idx] != '\0'; idx++) {
            printf("%c", buf[idx]);
          }
          ssize_t w = write(fd, buf, r);
          if (w == 0) {
            perror("Cannot send back echo");
          }
        }
      }
    }
  }

  return 0;
}
