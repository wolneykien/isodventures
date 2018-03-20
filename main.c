
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "spritemap.h"
#include "keygrab.h"
#include "dbg.h"

#define MAP_SIZE 20

int main( int argc, char **argv )
{
  srand( time( 0 ) );
  
  int sp_size = get_sprite_size();
  int scr_w = get_screen_width();
  int scr_h = get_screen_height();
  
  uint8_t *screen = screen_alloc();

  uint8_t *fb;
  int kbd_fd;
  
  if ( argc > 1 ) {
    fb = screen_mmap( argv[1] );
  } else {
    fb = screen_mmap( "/dev/fb0" );
  }

  if ( argc > 2 ) {
    kbd_fd = open_kbd( argv[2] );
  } else {
    kbd_fd = open_kbd( "/dev/input/event0" );
  }

  sprite_t *sprites[] = {
    load_sprite( "sprites/box.bmp" ),
    load_sprite( "sprites/box1.bmp" ),
    load_sprite( "sprites/box2.bmp" ),
    load_sprite( "sprites/box3.bmp" ),
    load_sprite( "sprites/box4.bmp" ),
    load_sprite( "sprites/box5.bmp" ),
    load_sprite( "sprites/box6.bmp" ),
    load_sprite( "sprites/man1.bmp" ),
    load_sprite( "sprites/man2.bmp" ),
    load_sprite( "sprites/man3.bmp" ),
    load_sprite( "sprites/man4.bmp" ),
    load_sprite( "sprites/man.bmp" ),
    load_sprite( "sprites/rocket.bmp" ),
    load_sprite( "sprites/rocket2.bmp" ),
  };

  int map[MAP_SIZE][MAP_SIZE], man_map[MAP_SIZE][MAP_SIZE];
  int i, j;

  for ( j = 0; j < MAP_SIZE; j++ ) {
    for ( i = 0; i < MAP_SIZE; i++ ) {
      map[j][i] = rand() % 7 + 1;
      man_map[j][i] = 0;
    }
  }

  int pos_h = 10;
  int pos_v = 10;
  
  man_map[pos_v][pos_h] = 9;

  int rocket_h = rand() % MAP_SIZE;
  int rocket_v = rand() % MAP_SIZE;
  man_map[rocket_v][rocket_h] = 13;
  map[rocket_v][rocket_h] = 1;

  
  int vx = - (scr_w / 2 - sp_size / 2);
  int vy = scr_h / 2 - sp_size / 2;

  int joffs_h = 0;
  int joffs_v = 0;

  int level = 0;
  int fall = 0;
  int win  = 0;
  
  for ( ;; ) {
    fill_screen( screen, 0 );
    
    if ( fall ) {
      put_sprite( sprites[11],
                  scr_w / 2 - sp_size / 2 + joffs_h,
                  scr_h / 2 - sp_size / 2 - joffs_v,
                  screen );
      fall += 10;
    }
    
    draw_sprite_map( vx - joffs_h, vy - joffs_v + fall,
                     MAP_SIZE, MAP_SIZE, map, sprites, screen );
    draw_sprite_map( vx - joffs_h, level + vy - joffs_v + fall,
                     MAP_SIZE, MAP_SIZE, man_map, sprites, screen );
    display_screen( screen, fb );

    if ( joffs_h != 0 ) {
      if ( joffs_h > 0 )
        joffs_h--;
      else
        joffs_h++;
    }
    
    if ( joffs_v != 0 ) {
      if ( joffs_v > 0 )
        joffs_v--;
      else
        joffs_v++;
    }
    
    usleep( 100000 );

    for ( j = 0; j < MAP_SIZE; j++ ) {
      for ( i = 0; i < MAP_SIZE; i++ ) {
        if ( i == rocket_h && j == rocket_v )
          continue;
        if ( map[j][i] > 0 && map[j][i] < 8 ) {
          if ( ( rand() % 32 ) == 3 ) {
            map[j][i] = map[j][i] + 1;
            if ( map[j][i] == 8 ) {
              map[j][i] = 0;
            }
          }
        } else {
          if ( map[j][i] == 0 && man_map[j][i] != 0 ) {
            dbg( "FALL!\n" );
            man_map[j][i] = 0;
            fall = 1;
          }
          if ( ( rand() % 16 ) == 15 ) {
            map[j][i] = rand() % 7 + 1;
          }
        }
      }
    }

    if ( man_map[pos_v][pos_h] == 13 || man_map[pos_v][pos_h] == 14 ) {
      man_map[pos_v][pos_h] = 14;
      level++;
    }

    char key = get_last_key( kbd_fd );

    if ( fall || win ) {
      // restart
      if ( key == KEY_ENTER ) {   
        for ( j = 0; j < MAP_SIZE; j++ ) {
          for ( i = 0; i < MAP_SIZE; i++ ) {
            map[j][i] = rand() % 7 + 1;
            man_map[j][i] = 0;
          }
        }

        pos_h = 10;
        pos_v = 10;
        
        man_map[pos_v][pos_h] = 9;

        rocket_h = rand() % MAP_SIZE;
        rocket_v = rand() % MAP_SIZE;
        man_map[rocket_v][rocket_h] = 13;
        map[rocket_v][rocket_h] = 1;

        
        vx = - (scr_w / 2 - sp_size / 2);
        vy = scr_h / 2 - sp_size / 2;

        joffs_h = 0;
        joffs_v = 0;

        level = 0;
        fall = 0;
        win  = 0;
      }
      continue;
    }
    
    if ( key != 0 ) {   
      switch ( key ) {
      case KEY_DOWN:
        dbg2( "JUMP S!\n" );
        if ( pos_v == (MAP_SIZE - 1) ) {
          dbg2( "S BORDER\n" );
          break;
        }
        if ( map[pos_v + 1][pos_h] == 0 ) {
          dbg2( "S BLOCKED\n" );
          break;
        }
        man_map[pos_v][pos_h] = 0;
        pos_v++;
        if ( man_map[pos_v][pos_h] == 13 ) {
          dbg( "WIN!\n" );
          win = 1;
        } else {
          man_map[pos_v][pos_h] = 8;
        }
        joffs_h = -get_hstep();
        joffs_v = get_vstep();
        vx -= get_hstep();
        vy += get_vstep();
        break;
      case KEY_RIGHT:
        dbg2( "JUMP E!\n" );
        if ( pos_h == (MAP_SIZE - 1) ) {
          dbg2( "E BORDER\n" );
          break;
        }
        if ( map[pos_v][pos_h + 1] == 0 ) {
          dbg2( "E BLOCKED\n" );
          break;
        }
        man_map[pos_v][pos_h] = 0;
        pos_h++;
        if ( man_map[pos_v][pos_h] == 13 ) {
          dbg( "WIN!\n" );
          win = 1;
        } else {
          man_map[pos_v][pos_h] = 9;
        }
        joffs_h = get_hstep();
        joffs_v = get_vstep();
        vx += get_hstep();
        vy += get_vstep();
        break;
      case KEY_LEFT:
        dbg2( "JUMP W!\n" );
        if ( pos_h == 0 ) {
          dbg2( "W BORDER\n" );
          break;
        }
        if ( map[pos_v][pos_h - 1] == 0 ) {
          dbg2( "W BLOCKED\n" );
          break;
        }
        man_map[pos_v][pos_h] = 0;
        pos_h--;
        if ( man_map[pos_v][pos_h] == 13 ) {
          dbg( "WIN!\n" );
          win = 1;
        } else {
          man_map[pos_v][pos_h] = 10;
        }
        joffs_h = -get_hstep();
        joffs_v = -get_vstep();
        vx -= get_hstep();
        vy -= get_vstep();
        break;
      case KEY_UP:
        dbg2( "JUMP N!\n" );
        if ( pos_v == 0 ) {
          dbg2( "N BORDER\n" );
          break;
        }
        if ( map[pos_v - 1][pos_h] == 0 ) {
          dbg2( "N BLOCKED\n" );
          break;
        }
        man_map[pos_v][pos_h] = 0;
        pos_v--;
        if ( man_map[pos_v][pos_h] == 13 ) {
          dbg( "WIN!\n" );
          win = 1;
        } else {
          man_map[pos_v][pos_h] = 11;
        }
        joffs_h = get_hstep();
        joffs_v = -get_vstep();
        vx += get_hstep();
        vy -= get_vstep();
        break;
      }
    }
  }

  for ( i = 0; i < 7; i++ )
    free_sprite( sprites[i] );
  
  screen_munmap( fb );
  screen_free( screen );
  close_kbd( kbd_fd );
}
