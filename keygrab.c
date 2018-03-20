#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <linux/input.h>

#include "keygrab.h"
#include "dbg.h"

int open_kbd( const char *kbd_name )
{
  int kbd_fd = open( kbd_name, O_RDONLY | O_NDELAY );

  if( kbd_fd < 0 )
  {
    fprintf( stderr, "Error opening keycode device/file: %s - %s\n",
             kbd_name, strerror( errno ) );
    return -1;
  }

  dbg( "Open keyboard device %s as %i\n", kbd_name, kbd_fd );
  
  return kbd_fd;
}

char get_last_key( int kbd_fd )
{
  struct et_input_event ev[64];
  char key = 0;
  int i;

  ssize_t rd = read( kbd_fd, ev, sizeof(ev) );
  
  if ( rd >= (int) sizeof(ev[0]) ) {
    for (i = 0; i < rd / sizeof(ev[0]); i++) {
      //uint16_t code;
      if ( ev[i].value > 0 && ev[i].type == EV_KEY ) {
        key = ev[i].code;
      }
    }
  }

  return key;
}

void close_kbd( int kbd_fd )
{
    close( kbd_fd );
}
