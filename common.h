#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

void sf_init(void);
double get_sf(double t);
double get_h(double t);
double get_diameter(double t);

#endif
