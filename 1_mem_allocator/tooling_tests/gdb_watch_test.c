// gcc -g gdb_watch_test.c
// gdb -tui ./a.out
int main() {
  // start
  int x = 10; // watch x ; continue
  int y = 20;

  x = 50;
  // HERE WILL SAY:
  // hardware watch
  // old value: 10
  // new value: 50
  y += x - 20;
  x = y;
  // HERE WILL SAY NOTHING
  // x does not change, still 50
  // Even when apparently should
  // if value does not change, it stays the same

  y++;
  x = 2;
  // HERE WILL SAY:
  // hardware watch
  // old value: 30
  // new value: 2

  return 0;
}
