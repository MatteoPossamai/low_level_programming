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

void print_s(std::string in, int index) {
  std::cout << "RECV" << index << ": " << in << std::endl;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cout << "Usage: " << argv[0] << "<index>";
  }
  auto index = atoi(argv[1]);

  int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (!udp_sock) {
    perror("Socket");
    return -1;
  }
  struct sockaddr_in addr;
  int res;
  int reuse_addr = 1;
  res = setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
                   sizeof(reuse_addr));
  if (res < 0) {
    perror("SO_REUSEADDR");
    return -1;
  }
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(SERVER_PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  res = bind(udp_sock, (struct sockaddr *)&addr, sizeof(addr));
  if (res != 0) {
    perror("Bind");
    return -1;
  }

  struct ip_mreq mreq;
  mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
  mreq.imr_interface.s_addr = inet_addr(INTERFACE_ADDR);
  res =
      setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
  if (res < 0) {
    perror("setsockopt");
    return -1;
  }
  bool subscribed = true;
  print_s("Subscribed: TRUE", index);

  unsigned int addrlen;
  addrlen = sizeof(addr);
  unsigned int counter = 0;

  size_t last_received = 0;

  while (true) {
    char message[50];

    if (counter % 20 == 0 && !subscribed) {
      // Subscribe in some cases
      res = setsockopt(udp_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                       sizeof(mreq));
      if (res < 0) {
        perror("setsockopt");
        return -1;
      }
      subscribed = true;
      print_s("Subscribed: TRUE", index);
    } else if (subscribed && counter % 10 == 0 && counter != 0) {
      res = setsockopt(udp_sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq,
                       sizeof(mreq));
      if (res < 0) {
        perror("setsockopt");
        return -1;
      }
      subscribed = false;
      print_s("Subscribed: FALSE", index);
    }

    if (subscribed) {
      res = recvfrom(udp_sock, message, sizeof(message), 0,
                     (struct sockaddr *)&addr, &addrlen);

      if (res < 0) {
        perror("Receive");
        return -1;
      }
      print_s(std::string("Received: ") + message, index);
      size_t recv_id = 0;
      int msg_idx = 0;
      while (msg_idx < res && message[msg_idx] != '|') {
        recv_id = recv_id * 10 + (message[msg_idx] - '0');
        msg_idx++;
      }

      if (recv_id > last_received + 1) {
        print_s(std::string("Detected gap, from ") +
                    std::to_string(last_received) + " to " +
                    std::to_string(recv_id),
                index);
      }
      last_received = recv_id;
    }
    sleep(1);
    counter++;
  }
  return 0;
}
