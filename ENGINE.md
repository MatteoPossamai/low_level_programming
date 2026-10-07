# Engine Project

- [Source code](./4_order_book_proj/)
  - [Components](./4_order_book_proj/src/)
    - [Allocator](./4_order_book_proj/src/allocator.hpp)
    - [Engine](./4_order_book_proj/src/engine.hpp)
    - [Queues](./4_order_book_proj/src/queues_impl.hpp)
    - [Messages](./4_order_book_proj/src/)
- [README](./4_order_book_proj/README.md)
- [Benchmarks](./4_order_book_proj/benchmark/README.md)
- [Design](./4_order_book_proj/DESIGN_DOC.md)

## Components Diagram

```shell

Clients -> Sockets -> [ Order Receiver ] -> [Queue]
                                                |
                                                V
                                          [Engine thread]
                                                |
                                                V
Clients <- Sockets  <- [Message Sender] <- [Queue]

```

## How to build

```shell
cd ./4_order_book_proj/
mkdir -p build && cd build
cmake .. && cmake --build .
```

## How to run

After building:

```shell
cd ./4_order_book_proj/build
./main
```

## Simplifications made

- Project is based on OUCH. Not all of it was implemented. Some cases and possibilities were ignored and part of the protocol slightly modified
- For network messages, was made the assumption that a TCP message was always the whole message, without the need to wait for other chunks.
- **Prices directly index the book array.** Price 0 is reserved, and prices must fit within `BUFFER_SIZE`
- Unfilled market orders are cancelled (coherent with OUCH).
- Receiver supplies account, and the account is linked to the TCP port used
- Outbound queue can be full. Did not dealt with the possibilities that queue might fill up
- Assumption that a user is linked to its TCP connection, and hence to its port and file descriptor. In case connection goes down, with the current design, the user is just unable to trade anymore.
- Once the engine goes down, all pending orders are forgotten/deleted

## Notes

Some missing features and Simplifications were made since the project is quite complex for the standard of the author,
when it comes to non trivial C++ code. This was a learning project that was able to expose me to:

1. `cmake`, compiling and linking in a decent size project
2. Custom allocators for the use case, and to handle memory
3. Lock free data structures (good first time :) )
4. Zero copy decoding of structure received over the wire
5. Some networking in practice
6. Data structures for fast(er) matching engine

Most of this might be trivial for some, but to me was already a big learning, so I am OK with the Simplifications made so far.
In the future I might do this from scratch again or implement the missing bits, to get it into a better shape. For now,
I'll carry on with learning and seeing some of this concepts better.
