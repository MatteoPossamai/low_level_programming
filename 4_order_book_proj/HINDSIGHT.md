# Project afterthoughts

Small file to record mistakes made, simplifications and what was surprising.

## Mistakes

### Mistake 1 - Low amount of design time

I spent relatively little amount of time thinking about the design of the overall
project, and this made it such that I had to do changes on the flight to the protocol
or other aspects of it. This forced me to later come back and think more about it,
while all the engineering work that I had already put into that went lost.

In the future, a better approach might be to put together a small and inefficient PoC
before committing to any choice. Can be also pseudocode, but need a higher degree of
certainty that:

1. Is going to work well with the design overall, with less changes
2. The difficulty on benchmarking accurately is know a priori

### Mistake 2 - Not thinking about ease of benchmarking while coding

Once the coding was done, came time to benchmark. And the API design of it made it relatively
hard to come up with a realistic and reliable benchmark. This required not little effort
in putting together something that was good enough, just to verify the code.

In the future, at least make the effort to see if there are ways to make the code easy to
benchmark, without sacrificing anything else that is desirable. Trying would give a bit more
perspective at least.

### Mistake 3 - Let codex do ANY of the debug

There were times in which code did not link, or there were `Segmentation Fault` and stuff like that.
After a couple of quite shallow attempts, sometimes I just shovelled my problems into `codex` or a
language model. This saved a lot of time, but probably removed me the struggle that make the learning
last longer.

In the future, I must accept that if I am coding for learning, then I should avoid `codex` to debug,
unless I have spent enough time and effort trying, or is something completely new for me.

### Mistake 4 - Rely on intuition for performance

For some optimization I tried to rely solely on my intuition. Most of the time, it was wrong, when
compared with measurements. ALWAYS MEASURE.

## Surprises

I was surprised how tiny edge cases can trigger a tail p99.9 delays that can be very bad in general,
and that those edge cases are not immediate to find out.

I got surprised by the fact that `io_uring` albeit more efficient in theory, if not properly used
becomes much slower than classic `epoll`.
