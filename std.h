/* date = April 13th 2022 3:16 pm */

#ifndef STD_H
#define STD_H

#define DEBUG

#include <stdio.h>
#include <stdlib.h>

#if !defined(__STDC__) || !defined(__STDC_VERSION__)
#error "C Standart not defined. (Either does not comply or isn\'t defined in compiler)"
#elif __STDC_VERSION__ < 1999L
#error "C Standart too low (Required C99 or above)"
#endif

#ifdef __GNUC__
#define INLINE static inline __attribute__((always_inline))
#else
#define INLINE static inline
#endif

#include <stdbool.h>

#ifdef DEBUG
#define STDF stdout
#else
#define STDF stderr
#endif

#define putsu(format) fprintf(STDF, format "\n");
#define puts(format) fprintf(STDF, "%s: " format "\n", __FILE__)
#define printfu(format, ...) fprintf(STDF, format, ##__VA_ARGS__);
#define printf(format, ...) fprintf(STDF, "%s: " format, __FILE__, ##__VA_ARGS__)

#define printfc(c, format, ...) (c)? printf(format, ##__VA_ARGS__) : 0

#define _putsc_2(c, format) ((c)? puts(format) : 0)
#define _putsc_3(c, format1, format2) ((c)? puts(format1) : puts(format2))
#define _putsc_2_3(a1, a2, a3, a4, ...) a4
#define _putsc_chooser(...) _putsc_2_3(__VA_ARGS__, _putsc_3, _putsc_2, )
#define putsc(...) _putsc_chooser(__VA_ARGS__)(__VA_ARGS__) 

// Statements and Declarations in Expressions is not standart in all compilers
// Therefore, calling safe(...) is not guarenteed to be evaluated to anything
// but a void, unless using GCC.
#define _safe_1(e) ({ int __INTERNEL_ERROR_CONTAINER; if ((__INTERNEL_ERROR_CONTAINER = e)) return __INTERNEL_ERROR_CONTAINER; __INTERNEL_ERROR_CONTAINER; })
#define _safe_2(e, format) ({ int __INTERNEL_ERROR_CONTAINER; if ((__INTERNEL_ERROR_CONTAINER = e)) { puts(format); return __INTERNEL_ERROR_CONTAINER; } __INTERNEL_ERROR_CONTAINER; })
#define _safe_3(e, format, val) ({ int __INTERNEL_ERROR_CONTAINER; if ((__INTERNEL_ERROR_CONTAINER = e)) { printf(format, val); return __INTERNEL_ERROR_CONTAINER; } __INTERNEL_ERROR_CONTAINER; })
#define _safe_1_2_3(a1, a2, a3, a4, ...) a4
#define _safe_chooser(...) _safe_1_2_3(__VA_ARGS__, _safe_3, _safe_2, _safe_1, )
#define safe(...) _safe_chooser(__VA_ARGS__)(__VA_ARGS__)

// Statements and Declarations in Expressions is not standart in all compilers
// Therefore, calling criticly_safe(...) is not guarenteed to be evaluated to anything
// but a void, unless using GCC.
#define _criticly_safe_1(e) ({ int __INTERNEL_ERROR_CONTAINER; if ((__INTERNEL_ERROR_CONTAINER = e)) abort(); else __INTERNEL_ERROR_CONTAINER; });
#define _criticly_safe_2(e, format) ({ int __INTERNEL_ERROR_CONTAINER; if ((__INTERNEL_ERROR_CONTAINER = e)) { puts("CRITICAL: " format); abort(); } __INTERNEL_ERROR_CONTAINER; })
#define _criticly_safe_1_2(a1, a2, a3, ...) a3
#define _criticly_safe_chooser(...) _criticly_safe_1_2(__VA_ARGS__, _criticly_safe_2, _criticly_safe_1, )
#define criticly_safe(...) _criticly_safe_chooser(__VA_ARGS__)(__VA_ARGS__)

#endif //STD_H