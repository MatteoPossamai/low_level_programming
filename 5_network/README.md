# Networking - Q5

**Status: complete for the agreed scope.** Reading completion was confirmed by
the author. This quarter used small socket experiments and packet inspection
to study networking after the Q4 matching engine.

## Reading scope

- [x] Kernel receive path.
- [x] TCP behaviour, including Nagle / delayed ACK and byte-stream framing.
- [x] UDP multicast for market data and application-level gap detection.
- [x] Kernel bypass concepts: DPDK/Onload, polling vs interrupts.

These mark the agreed learning scope, not a full-book read or an implementation
of each mechanism.

## Experiments

| Directory | Implementation | Main observation |
| --- | --- | --- |
| [`framing/`](framing/) | TCP client/server using `<message_len>\|<message>`; split and combined writes; `TCP_NODELAY`; packet capture | A receive can contain part of a message or several messages. Framing state must survive across receives. |
| [`multi_thread_rx/`](multi_thread_rx/) | One epoll receiver, two TCP connections, a buffer per connection; a sender thread generates input | One receiver event loop can assemble data from multiple connections independently. |
| [`udp_mcast/`](udp_mcast/) | Multicast publisher/subscribers, numbered messages, join/leave experiments, gap detection, packet capture | One multicast stream can reach multiple subscribers; the application must detect missing messages. |

Despite the `multi_thread_rx` name, the receive path uses one thread.
The multicast program detects gaps; it does not implement recovery or replay.
The examples are learning artifacts, not production networking implementations.

## Packet inspection

- [`framing/capture.pcap`](framing/capture.pcap) - TCP capture.
- [`udp_mcast/capture.pcap`](udp_mcast/capture.pcap) - UDP multicast capture.
- [`Wireshark and tcpdump notes`](../0_tools_n_specs/WIRESHARK.md) - filters,
  stream boundaries, sequence numbers, and checksum offloading.

## Build layout

`udp_mcast/` has its own `CMakeLists.txt`, building `publisher` and `subscriber`.
The interface address is configured in `udp_mcast/common.hpp`.
`framing/` and `multi_thread_rx/` contain standalone C++ sources.
Compilation workflow: [`compile-cpp-reproducibly`](../skills/compile-cpp-reproducibly/SKILL.md).

## Research pointers

Optional follow-up, outside the completed quarter:

| Start here | Question |
| --- | --- |
| [Homa/Linux (ATC 2021)](https://www.usenix.org/conference/atc21/presentation/ousterhout) | Can a message-oriented transport reduce short-message delays under load? Its evaluation also identifies host software overhead as a remaining bottleneck. |
| [eRPC (NSDI 2019)](https://www.usenix.org/conference/nsdi19/presentation/kalia) | How close can a userspace RPC library on commodity CPUs and Ethernet get to specialized systems? |
| [Caladan (OSDI 2020)](https://www.usenix.org/conference/osdi20/presentation/fried) | How can CPU scheduling and shared-resource interference delay a request even when the network is fast? |

These are datacenter studies; their measured gains depend on workload and
hardware. They support the existence of software and queueing overhead, not
a universal speedup or a way around physical link limits.

For the ZeroMQ connection, its author's [architecture chapter, section 24.6](https://aosabook.org/en/v2/zeromq.html)
explains batching and the tradeoff between stack-processing cost and latency.
Homa also has an [individual Internet-Draft dated 20 October 2026](https://datatracker.ietf.org/doc/draft-ousterhout-tsvwg-homa/);
it is work in progress, not an IETF standard.

## Next quarter

Q6 is planned as [SIMD/image processing and persistent storage](../6_simd_n_storage/README.md).
Further networking research is a possible follow-up, not an unfinished
requirement of Q5.
