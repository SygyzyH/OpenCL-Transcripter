/* date = January 30th 2022 2:46 pm */

#ifndef ARGS_H
#define ARGS_H

#include <stdio.h>

extern int sets;

// assert(N < sizeof(int));
enum SETTINGS { OK, RT, DB, FB, CS, OUT_TXT };
#define setset(i, set) i |= 1 << set
#define clrset(i, set) i &= ~(1 << set)
#define chkset(i, set) i & (1 << set)

int hndl_set(int argc, char *argv[]);
char* ldfile(char const *filepath);

#endif //IO_H
