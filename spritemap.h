
#include <stdint.h>

#define PIXEL_SIZE 2
#define SPRITE_SIZE 64
#define SPRITE_HSTEP 20
#define SPRITE_VSTEP 10
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

struct sprite_stroke {
  int blank;
  int paint;
};

typedef struct sprite_stroke sprite_stroke_t;
  
struct sprite {
  uint8_t *buf;
  sprite_stroke_t *strokes;
};

typedef struct sprite sprite_t;

int get_pixel_size();
int get_sprite_size();
int get_hstep();
int get_vstep();
sprite_t *load_sprite( const char *sprite_bmp );
void free_sprite( sprite_t *sprite );
int get_screen_width();
int get_screen_height();
void put_sprite( sprite_t *sprite, int x, int y, uint8_t *screen_buf );
uint8_t *screen_alloc();
void screen_free( uint8_t *screen_buf );
void fill_screen( uint8_t *screen_buf, int c );
void display_screen( uint8_t *screen_buf, uint8_t *framebuf );
uint8_t *screen_mmap( const char *fbname );
int screen_flush(  uint8_t *framebuf );
int screen_munmap( uint8_t *framebuf );
void draw_sprite_map( int vx, int vy, int map_cols,
                      int map_rows, int map[map_cols][map_rows],
                      sprite_t *sprites[], uint8_t *screen_buf );
