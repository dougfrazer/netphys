#pragma once

#include <assert.h>
#include <cmath>
#include <stdlib.h>
#include <cstring>


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

#ifndef FLT_EPSILON
#define FLT_EPSILON      1.192092896e-07F        // smallest such that 1.0+FLT_EPSILON != 1.0
#endif

#define KINDA_CLOSE_ENOUGH 0.00001f

// todo: see christer ericson's 2005 GDC talk for better implementations
static inline bool FloatEquals(float a, float b) { return fabsf(a - b) < (KINDA_CLOSE_ENOUGH * max(1.0f, max(fabsf(a), fabsf(b)))); }
static inline bool different_sign(float a, float b) { return a * b < 0.0f; }
static inline bool same_sign(float a, float b) { return a * b >= 0.0f; }