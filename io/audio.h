/* date = January 31st 2022 5:35 pm */
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "pwave.h"

#ifndef AUDIO_H
#define AUDIO_H

#define NUM_BUF 3
#define SAMPLERATE 44100
#define BITRATE 16
#define FRAMESPERSECOND 20
#define CLICAP 30
#define CLICHR '\xdb'

int auinit();
int austrt();
int aucln();
int playwav(WAVC *file);

#endif //AUDIO_H
