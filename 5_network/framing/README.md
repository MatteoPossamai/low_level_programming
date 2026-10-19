# Client/Server experiment

## Protocol

Tried to create a simple client and server such that
we can create messages with the following protocol:

```c++
<message_len>|<message>
```

So the client sends the message length, followed by pipe `|`
and followed by a message of that length.

## Catch

TCP might decide to put it all under the same message. These
are two runs done with the same code:

```shell
❯ g++ framing/server.cpp -o server.out && ./server.out
Server listening on port 8080
Buffer: 5|hello5|abc
hello
Buffer: de
abcde
Buffer: 3|abc5|hello10|hellohello
abc
hello
hellohello
Buffer:

❯ ./server.out
Server listening on port 8080
Buffer: 5|hello5|abcde3|abc5|hello10|hellohello
hello
abcde
abc
hello
hellohello
Buffer:
```

This is an effect of the STREAM interface, so it is a timing matter. You will not be able
to tell when this is going to happen. The below just makes it sure that the sender does not
buffer small packages. Hence, given that is faster to fan out, makes it more possible that the
server will see it as a unique blob.

```c
  int flag = 1;
  setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
```
