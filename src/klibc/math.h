#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

float clamp(float val, float min, float max);
int powi(int x, int y);
double pow(double x, double y);
double ceil(double x);
double log(double y);
double exp(double x);
double fabs(double x);
int max(int a, int b);
int min(int a, int b);
float fminf(float a, float b);
double fmin(double a, double b);
float fmaxf(float a, float b);
double fmax(double a, double b);

#ifdef __cplusplus
}
#endif

#endif

