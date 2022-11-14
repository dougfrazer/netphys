#pragma once

#include <assert.h>
#include <cmath>
#include <stdlib.h>
#include <cstring>


static inline bool FloatEquals(float a, float b) { return fabs(a-b) < 0.0001f; }
static inline bool different_sign(float a, float b) { return a * b < 0.0f; }
static inline bool same_sign(float a, float b) { return a * b >= 0.0f; }

#ifndef PI
#define	PI					3.1415926535897932384626433832795028841971693993751f
#endif

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef clamp
#define clamp(x,a,b)        (max(a,min(x,b)))
#endif