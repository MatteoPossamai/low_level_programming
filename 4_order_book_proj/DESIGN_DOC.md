# Design Doc

## Components

### Inbound Message Receiver

Web server that is able to get in network messages and serialize them so that they
are readable from the next components. Is the first component in the pipeline, and
will mainly wait on messages over TCP and cast them via zero copy. Once serialization
is done, the message is put into a Queue.

There are different connections, and all of the workflows are independent
from one another (reading message, serialize, put into queue).

### Matching Engine

Here is where messages coming in from the network are then put into the match to
see if bid and ask match and consequently causes a fill. It is going to read the
inbound from the Queue of the Inbound Message Receiver and feed it to the algorithm.

Once there is a fill, or another event, that message is forwarded to another
queue for notification purposes to the outside world.

This is going to be implemented with a flat Array with each spot corresponding
to the price it refers to, with a slightly improved structure from
[data structure work](./data_structure_bench/tick_offset_str.cpp).

### Outbound message Writer

The equivalent of the Inbound Message Receiver, but for the opposite purpose.
All of the events that come out of the Matching Engine will then need to be
fanned out of the system so that users of it are going to see what is going on.

Drop-copy is private per firm: a firm only sees its own fills. Events are routed
by owning firm, then broadcast to every drop-copy session of that firm. One SPSC
queue per session.

## Threading model

### Inbound Message Receiver - Threading Model

As specified above, receiving and putting elements into a queue, when those
are independent and there is not direct causality, means that we can employ
multi-threading to handle connections in this particular component.

This would mean that the queue will need to be thread safe.

### Matching Engine - Threading Model

This is the hot path. Here is when order starts to matter, and the order in
which elements arrived on the queue is relevant. Any future operation depends
on the effect that the previous one had. So one operation might have different
outcome if the previous trade was a big one that moved the price in either direction.

This means that this path must be single threaded. This will ensure coherence
and total ordering that is deterministic and not casual. This is the reason
this part will be single threaded.

### Outbound message Writer - Threading Model

Multi-threaded: each writer thread owns a set of sessions and drains their SPSC
queues. Sessions are independent, so sending is parallel.

### Notes on Threading Model

Inbound uses one MPSC queue (many receivers -> engine). Outbound uses one SPSC
queue per session (engine -> writer), not SPMC: SPMC hands each event to exactly
one consumer, while drop-copy needs every event delivered to every session of the
owning firm.

## Failure Handling

### Invalid messages

Given that the Matching Engine is the hot path, and that is single threaded,
while Inbound is parallel, the checks for validity of messages will be part
of the Inbound pipeline, so that the engine can assume that what is provided
is valid and hence be faster.

## Matching Engine Rules

1. Priority of an order is: Price->Time;
2. Order types in scope: Limit, Market, Cancel;
3. Partial fills are available, where one order fills only part of another;
4. Time in force supported for now is only Day, as long as engine works;
