# Difference with optimization

Compiled with

```shell
gcc -O0 -S -masm=intel gdb_watch_test.c -o out_O0
gcc -O2 -S -masm=intel gdb_watch_test.c -o out_O2
```

See diff:

```shell
nvim -d out_O0 out_O2
```

What is happening:

- With `-O0` compiler puts all the operation explicitly loading from memory as in the code
- With `-O2` compiler sees that is all pointless, so just does nothing at all
