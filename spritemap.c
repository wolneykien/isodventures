
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "spritemap.h"
#include "dbg.h"

int get_pixel_size()
{
  return PIXEL_SIZE;
}

int get_sprite_size()
{
  return SPRITE_SIZE;
}

#define BMP_HEADER_SIZE 70

static void make_sprite_strokes( sprite_t *sprite )
{
  int sp_size = get_sprite_size();
  int ps = get_pixel_size();
  
  sprite->strokes = malloc( sp_size * sp_size * sizeof ( sprite_stroke_t ) );
  if ( sprite->strokes == NULL ) {
    perror( "ERROR: Unable to allocate memory for strokes" );
    return;
  }
  
  int y, x;
  for ( y = 0; y < sp_size; y++ ) {
    dbg3( "%02i:", y );
    sprite_stroke_t *stroke = sprite->strokes + y * sp_size;
    stroke->blank = 0;
    stroke->paint = 0;
    
    for ( x = 0; x < sp_size; x++ ) {
      uint16_t *pix = (uint16_t *) (sprite->buf + ( y * sp_size + x ) * ps);
      if ( *pix == 0xffff ) {
        if ( stroke->paint > 0 ) {
          dbg3( " %i/%i", stroke->blank, stroke->paint );
          stroke++;
        }
        stroke->blank++;
      } else {
        stroke->paint++;
      }
    }

    dbg3( " %i/%i\n", stroke->blank, stroke->paint );
  }
}

sprite_t *load_sprite( const char *sprite_bmp )
{
  int fd = open( sprite_bmp, 0 );
  if ( fd < 0 ) {
    fprintf( stderr, "ERROR: Unable to read the sprite from %s\n",
             sprite_bmp );
    return NULL;
  }

  int bufsize = get_sprite_size() * get_sprite_size() * get_pixel_size();
  uint8_t *buf = malloc( bufsize );

  off_t offs = lseek( fd, BMP_HEADER_SIZE, SEEK_SET );
  if ( offs != BMP_HEADER_SIZE ) {
    perror( "ERROR: Unable to skip the BMP header" );
    free( buf );
    buf = NULL;
  }

  if ( buf ) {
    ssize_t rd = read( fd, buf, bufsize );
    if ( rd < bufsize ) {
      fprintf( stderr, "ERROR: Unable to read %u sprite bytes from %s\n",
               bufsize, sprite_bmp );
      free( buf );
      buf = NULL;
    }
  }

  sprite_t *sprite = malloc( sizeof( sprite_t ) );
  
  if ( sprite ) {
    sprite->buf = buf;
    sprite->strokes = NULL;
    make_sprite_strokes( sprite );
  } else {
    perror( "ERROR: Unable to allocate a sprite" );
    free( buf );
  }

  close( fd );

  dbg( "\"%s\" loaded\n", sprite_bmp );

  return sprite;
}

void free_sprite( sprite_t *sprite )
{
  if ( sprite ) {
    free( sprite->buf );
    free( sprite->strokes );
    free( sprite );
  }
}

int get_screen_width()
{
  return SCREEN_WIDTH;
}

int get_screen_height()
{
  return SCREEN_HEIGHT;
}

static void put_sprite_line( int xorig, uint8_t *pix_line,
                             sprite_stroke_t *stroke_line,
                             uint8_t *screen_line )
{
  int ps = get_pixel_size();
  int sp_size = get_sprite_size();
  int cols = get_screen_width();

  if ( stroke_line->blank == 0 && stroke_line->paint == 0 ) {
    dbg3( " blank\n" );
    return;
  }
  
  dbg2( "scr line: %p, sp line: %p\n", screen_line, pix_line );

  int x = 0;
  while ( x < sp_size ) {
    x += stroke_line->blank;
    dbg3( "->%i", x + xorig );
    if ( x < sp_size ) {
      int paint = stroke_line->paint;
      if ( (xorig + x + paint - 1) > cols ) {
        paint -= ((xorig + x + paint) - cols);
      }
      int xoffs = 0;
      if ( xorig < 0 ) {
        xoffs = -xorig;
        paint += xorig;
      }
      if ( paint > 0 ) {
        memcpy( screen_line + (xorig + xoffs + x) * ps,
                pix_line + ( xoffs + x ) * ps,
                paint * ps );
      }
      x += stroke_line->paint;
      stroke_line++;
      dbg3( ":%i", xorig + x );
    }
  }

  dbg3( "\n" );
}

void put_sprite( sprite_t *sprite, int x, int y, uint8_t *screen_buf )
{
  int ps = get_pixel_size();
  int cols = get_screen_width();
  int rows = get_screen_height();
  int sp_size = get_sprite_size();

  dbg2( "%s( %p, %i, %i, %p )\n", __func__, sprite, x, y, screen_buf );
  
  if ( (x + sp_size) < 0 || x >= cols ||
       (y + sp_size) < 0 || y >= rows)
  {
    return;
  }

  int yoffs = 0;
  if ( y < 0 ) {
    yoffs = -y;
    y = 0;
  }
  
  int addr = ps * cols * y;
  
  uint8_t *pix_line = sprite->buf + (yoffs * sp_size * ps);
  sprite_stroke_t *stroke_line = sprite->strokes + (yoffs * sp_size);
  uint8_t *screen_line = screen_buf + addr;

  int line, row;
  for ( line = yoffs, row = y; line < sp_size && row < rows; line++, row++ ) {
    put_sprite_line( x, pix_line, stroke_line, screen_line );
    pix_line += sp_size * ps;
    stroke_line += sp_size;
    screen_line += cols * ps;
  }
}

uint8_t *screen_mmap( const char *fbname )
{
  int ps = get_pixel_size();
  int cols = get_screen_width();
  int rows = get_screen_height();

  int fd = open( fbname, O_RDWR );
  if ( fd < 0 ) {
    fprintf( stderr, "ERROR: Unable to open frame buffer from %s\n",
             fbname );
    return NULL;
  }

  uint8_t *buf = mmap( NULL, rows * cols * ps, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0 );

  if ( buf == MAP_FAILED ) {
    perror( "ERROR: Unable to mmap screen buffer" );
  }

  dbg( "Map frame buffer at %p\n", buf );

  close( fd );

  return buf;
}

int screen_flush(  uint8_t *framebuf )
{
  int ps = get_pixel_size();
  int cols = get_screen_width();
  int rows = get_screen_height();

  int ret = msync( framebuf, rows * cols * ps, MS_ASYNC );
  if ( ret < 0 ) {
    perror( "ERROR: Unable to sync the screen buffer" );
  }

  return ret;
}

int screen_munmap( uint8_t *framebuf )
{
  int ps = get_pixel_size();
  int cols = get_screen_width();
  int rows = get_screen_height();

  int ret = munmap( framebuf, rows * cols * ps );
  if ( ret < 0 ) {
    perror( "ERROR: Unable to munmap the screen buffer" );
  }

  return ret;
}

int get_hstep() {
  return SPRITE_HSTEP;
}

int get_vstep() {
  return SPRITE_VSTEP;
}

void draw_sprite_map( int vx, int vy, int map_cols,
                      int map_rows, int map[map_cols][map_rows],
                      sprite_t *sprites[], uint8_t *screen_buf )
{ 
  //int sp_size = get_sprite_size();
  int hstep = get_hstep();
  int vstep = get_vstep();
  
  int i, j;

  int voffs = 0;
  int hoffs = 0;
  for ( j = 0; j < map_rows; j++ ) {
    voffs = j * vstep;
    hoffs = -j * hstep;
    for ( i = 0; i < map_cols; i++ ) {
      int sprite_num = map[j][i];
      if ( sprite_num > 0 ) {
        put_sprite( sprites[ sprite_num - 1], hoffs + i * hstep - vx,
                    voffs - vy, screen_buf );
      }
      voffs += vstep;
    }
  }
}

uint8_t *screen_alloc()
{
  int ps = get_pixel_size();
  int cols = get_screen_width();
  int rows = get_screen_height();

  return malloc( ps * cols * rows );
}

void screen_free( uint8_t *screen_buf )
{
  free( screen_buf );
}

void fill_screen( uint8_t *screen_buf, int c )
{
  int ps = get_pixel_size();
  int cols = get_screen_width();
  int rows = get_screen_height();
  
  memset( screen_buf, c, ps * cols * rows );
}

void display_screen( uint8_t *screen_buf, uint8_t *framebuf )
{
  int ps = get_pixel_size();
  int cols = get_screen_width();
  int rows = get_screen_height();

  memcpy( framebuf, screen_buf, ps * cols * rows );
  screen_flush( framebuf );
}
