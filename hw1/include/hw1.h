#ifndef HW_H
#define HW_H

#include "audio.h"
#include "const.h"
#include "myrand.h"

int numargs;
int factor;
int numargbsize;
int newannotationsize;


int framecopy(int* fp, int* fp2, int channels, int bytes_per_sample);

int interpolate(int* currentframe, int* previousframe, int channels, int bytes_per_sample, int factor);

int encrypt(int* inputframe, int channels, int bytes_per_sample, int factor);

#endif
