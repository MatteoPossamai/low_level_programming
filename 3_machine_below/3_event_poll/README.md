# `io_uring` model

`io_uring` uses simply two different ring buffer, one
is basically a producer of events to handle for
the kernel and one produces the responses that the
kernel emits. These are zero copy buffers, that allow
`io_uring` to be very performant and to reduce `syscall`s and,
importantly, their overhead.

So is a two way communication buffer, that can be collapsed into
a request-response kind of paradigm, where the user space
sends to the kernel some tasks and asynchronously get a response.
