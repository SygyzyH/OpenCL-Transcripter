/* date = April 13th 2022 3:16 pm */

#ifndef STD_H
#define STD_H

#define puts(...) printf("%s: ", __FILE__); puts(__VA_ARGS__);
#define printf(format, ...) printf("%s: " format, __FILE__)

#endif //STD_H
