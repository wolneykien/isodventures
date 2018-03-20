#include <stdio.h>

#ifdef DEBUG
#  define dbg( ... )  fprintf( stderr, __VA_ARGS__ )
#else
#  define dbg( ... )
#endif

#ifdef DEBUG2
#  define dbg2( ... )  fprintf( stderr, __VA_ARGS__ )
#else
#  define dbg2( ... )
#endif

#ifdef DEBUG3
#  define dbg3( ... )  fprintf( stderr, __VA_ARGS__ )
#else
#  define dbg3( ... )
#endif
