#ifndef MATH_H
#define MATH_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

int powi(int x, int y);
long long powl(long long x, long long y);
/*
float fminf(float a, float b);
double fmin(double a, double b);
float fmaxf(float a, float b);
double fmax(double a, double b);
float clamp(float val, float min, float max);
double pow(double x, double y);
double ceil(double x);
double log(double y);
double exp(double x);
double fabs(double x);
*/
int max(int a, int b);
int min(int a, int b);

#ifdef __cplusplus
}
#endif

#endif
