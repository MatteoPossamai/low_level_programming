# UDP multicast

## Market data feed

Given the high volume and high number of subscriber, having a market feed
based off of TCP is not scalable. UDP multicast is a better choice that
can support higher throughput on the producer side.

In case a packet is lost or arrives out of order, the other hand (consumer)
at the application level has to detect the gap (similarly to what was done in
[./subscriber.cpp]).

To recover, the only realistic way is to have a out of bound channel in which
the application asks for sending again the packets that were not received before.
In this case, it is necessary that the user gives the ID after which he needs messages again.

## `tcpdump`

The result is quite simple and underwhelming. You can see all of the messages that were on flight,
but there is no additional flags and `ACK` as for TCP.

## Code

### Publisher

Just need to decide on one of the IP addresses that are reserved for UDP multicast. No need to bind,
just, once that address and port are chosen, just fire with `sendto`. Note that the port can be
0 so that there is one that the OS gives out at random. This is harder for the subscriber though.

### Subscriber

This is trickier. Needs to create a UDP socket, set it to `SO_REUSEADDR` so that multiple one on the same
host can connect to the same port for listening. Then bind the address, `IP_ADD_MEMBERSHIP` on a specific
network interface supporting it. Then can get message with `recvfrom`.
