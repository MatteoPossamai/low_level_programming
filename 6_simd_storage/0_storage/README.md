# SDB

Simple Data Base. A hand-made implementation of a database for learning purpose.
The database is a simple key value store.

## Interface

- `put(key, value)`: writes or updates values at that key. Should always succeed unless hardware failures
- `get(key)`: reads the value of a given key. If the key is not provided will signify with Algebraic data types.
- `del(key)`: if key exists is deleted. If not, is a no-op

## Record format

```text
<key><value><CRC-32>
```

Key and value sizes come from their C++ types. CRC-32 occupies 4 bytes and
covers the stored key and value bytes. Files use native object representations;
reopen them with the same types and a compatible platform.

## Index

Following the Bitcask implementation, in RAM there will be an index. This index will be a map of its own, maintaining for each key the file index at which you can find the latest record, made up with key and value. Older records might be on disk, but they are not accessible by the index.

## Overwrites

In case a key is overwritten, the new value is inserted and the Index is updated to link just to the new one, and make the old one not accessible.

In case of delete, a new entry with a tombstone will be made, so that this is seen as missing key rather than seeing older values.

## Ownership

To simplify the access patterns, the ownership model will start with a single process, that has a single writer. There might be different reader threads, but this might change.

## Recovery

- On reopening, scan records and rebuild the index. The latest record wins.
- Truncate an incomplete final record before accepting more writes.
- Reject a complete record with a checksum mismatch, including at the end.
- Report replay read errors instead of truncating the file.

## Durability

Not implemented yet: writes are buffered, with no explicit disk sync.
Current success returns do not promise survival after a crash or power loss.

Required behaviour:

- Per-write syncing: write the record, flush any stream buffer, sync the file,
  then acknowledge success. Report write and sync failures.
- Batched syncing: sync a group of records. Only acknowledge them as durable
  after that sync succeeds. Compare latency with per-write syncing.
- A write interrupted before acknowledgement may survive or be lost.

Durability assumes the filesystem and device honour sync requests. File creation
also requires syncing the parent directory to persist the new filename.
Killing a process tests process failure, not OS failure or power loss.
Recovery tests, I/O checks, sync policies, and measurements are still pending.
