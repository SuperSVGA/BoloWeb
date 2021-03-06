//
// BoloWeb v0.8
// ©1996 Pavani Diwanji <pavani@CS.Stanford.EDU>
//
// BoloWeb is freeware. Providing that you acknowledge my authorship, 
// you may distribute it any way you like, or do anything else with it, 
// including producing derivative works from the source code. 
//

#define HEAPSCRAMBLE 1

#if HEAPSCRAMBLE
#define memcheck() do_memcheck()
#else
#define memcheck() 0
#endif

import void do_memcheck(void);
import void debug(char *format, ...);
