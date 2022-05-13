/* date = January 31st 2022 5:35 pm */
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "pwave.h"

#ifndef AUDIO_H
#define AUDIO_H

#define NUM_BUF 4
#define SAMPLERATE 16000
#define BITRATE 16
extern int FRAMESPERSECOND;
#define CLICAP 30
#define CLICHR '\xdb'

extern int aucurbuf;
extern char *classname;
extern WAVEFORMATEX formatex;

int auinit();
int austrt();
int aucln();
WAVEHDR augetb();
int playwav(WAVC *file, int blocking);

#endif //AUDIO_H
