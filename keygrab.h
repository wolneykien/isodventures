
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <linux/input.h>

struct et_input_event {
  uint64_t time;
  uint16_t type;
  uint16_t code;
  int32_t value;
} kbd_event;

int open_kbd( const char *kbd_name );
char get_last_key( int kbd_fd );
void close_kbd( int kbd_fd );
