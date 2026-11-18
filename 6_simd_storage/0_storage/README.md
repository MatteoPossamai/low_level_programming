# SDB

Simple Data Base. A hand-made implementation of a database for learning purpose.
The database is a simple key value store.

## Interface

- `put(key, value)`: writes or updates values at that key. Should always succeed unless hardware failures
- `get(key)`: reads the value of a given key. If the key is not provided will signify with Algebraic data types.
- `del(key)`: if key exists is deleted. If not, is a no-op

## Record format

A single entry in the disk file will look like:

```md
<key_length><key><value_length><value>
```

But given that types and length are implicitly given by the type the
templated class is implemented with, we can skip them and just use:

```md
<key><value>
```

## Index

Following the Bitcask implementation, in RAM there will be an index. This index will be a map of its own, maintaining for each key the file index at which you can find the latest record, made up with key and value. Older records might be on disk, but they are not accessible by the index.

## Overwrites

In case a key is overwritten, the new value is inserted and the Index is updated to link just to the new one, and make the old one not accessible.

In case of delete, a new entry with a tombstone will be made, so that this is seen as missing key rather than seeing older values.

## Ownership

To simplify the access patterns, the ownership model will start with a single process, that has a single writer. There might be different reader threads, but this might change.
