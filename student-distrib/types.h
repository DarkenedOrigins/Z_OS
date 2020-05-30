/* types.h - Defines to use the familiar explicitly-sized types in this
 * OS (uint32_t, int8_t, etc.).  This is necessary because we don't want
 * to include <stdint.h> when building this OS
 * vim:ts=4 noexpandtab
 */

#ifndef _TYPES_H
#define _TYPES_H

#define NULL 		(0)
#define INT_MAX 	(0x7FFFFFFF)
#define INT_MIN		(0x80000000)
#define UINT_MAX	(0xFFFFFFFF)
#define INT16_MAX   (32767)
#define INT16_MIN   (-32768)

#define MAX_TERMINALS (3)

#ifndef ASM

/* Types defined here just like in <stdint.h> */
typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

typedef unsigned char bool;
#define TRUE        (1)
#define FALSE       (0)

#endif /* ASM */

#endif /* _TYPES_H */
