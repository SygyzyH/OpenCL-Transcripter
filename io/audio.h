/* date = January 31st 2022 5:35 pm */

#ifndef AUDIO_H
#define AUDIO_H

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "pwave.h"

#define NUM_BUF 4
#define SAMPLERATE 16000
#define BITRATE 16
extern int FRAMESPERSECOND;
#define CLICAP 30
#define CLICHR '\xdb'

#define UINT_BITRATE uint##BIRATE##_t

extern WAVEFORMATEX formatex;

int auinit();
int austrt();
int aucln();
WAVEHDR augetb();
int playwav(WAVC *file, int blocking);
double computevu(double *signal, int sz);

#endif //AUDIO_H
