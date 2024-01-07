#ifndef UTILS_H_
#define UTILS_H_	1

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>

#define SWAP_NUMERIC(a, b)												\
	do {																\
		a ^= b;															\
		b ^= a;															\
		a ^= b;															\
	} while(0)

#define SWAP_ANY(a, b, type)											\
	do {																\
		type __var = a;													\
		a = b;															\
		b = __var;														\
	} while(0)

#define DIE(assertion, call_description)								\
	do {																\
		if (assertion) {												\
			fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);			\
			perror(call_description);									\
			exit(EXIT_FAILURE);											\
		}																\
	} while (0)

#define ARRAY_SIZE(static_arr)											\
	(sizeof((static_arr)) / sizeof(*(static_arr)))

#define DEF_KERNEL(var)													\
	int (var)[KERNEL_SIZE][KERNEL_SIZE]

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Limits @a val to the given range
*/
static int _clamp(double val, int min, int max)
{
	int int_val = (int)round(val);

	if (int_val < min)
		return min;
	if (int_val > max)
		return max;
	return int_val;
}

#ifdef __cplusplus
}
#endif

#endif
