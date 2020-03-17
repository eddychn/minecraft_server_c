#ifndef CSMATH_H
#define CSMATH_H
#include "vector.h"

#ifdef __GNUC__
#define Math_SqrtF(x) __builtin_sqrtf(x)
#else
API float Math_SqrtF(float x);
#endif

API float Math_Distance(Vec *v1, Vec *v2);
API void Math_Normalize(Vec *src, Vec *dst);
#endif
