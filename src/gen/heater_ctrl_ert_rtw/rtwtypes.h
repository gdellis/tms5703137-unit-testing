/*
 * File: rtwtypes.h
 *
 * HAND-WRITTEN STAND-IN for the rtwtypes.h that Simulink Embedded Coder emits.
 * It reproduces the layout and the type choices Embedded Coder makes for
 *   Hardware Implementation -> Device vendor: ARM Compatible, Device type: ARM Cortex-R
 * (char 8, short 16, int 32, long 32, pointer 32, big-endian) so that host tests see
 * the same names the target code uses. Replace this whole directory with the real
 * generated output; do not edit generated files.
 *
 * Note for host builds: on x86-64 gcc/clang `long` is 64-bit, so ulong_T is wider
 * here than on the TMS570. Embedded Coder maps int32_T/uint32_T to int/unsigned int,
 * which are 32-bit on both, so generated arithmetic is unaffected. Build with -m32 if
 * you want an exact ILP32 match.
 */
#ifndef RTWTYPES_H
#define RTWTYPES_H

/* Logical type definitions.
 * Embedded Coder guards these so that a preceding <stdbool.h> wins. If this file is
 * included *before* <stdbool.h>, <stdbool.h> redefines true/false (1U -> 1); gcc and
 * clang hide that because it happens inside a system header, other front ends and
 * any project header that defines true/false do not. Include hand-written headers
 * that use <stdbool.h> first (see docs/02, section 5). */
#if (!defined(__cplusplus))
#ifndef false
#define false                          (0U)
#endif

#ifndef true
#define true                           (1U)
#endif
#endif

/*=======================================================================*
 * Fixed width word size data types:                                     *
 *   int8_T, int16_T, int32_T     - signed 8, 16, or 32 bit integers     *
 *   uint8_T, uint16_T, uint32_T  - unsigned 8, 16, or 32 bit integers   *
 *   real32_T, real64_T           - 32 and 64 bit floating point numbers *
 *=======================================================================*/
typedef signed char int8_T;
typedef unsigned char uint8_T;
typedef short int16_T;
typedef unsigned short uint16_T;
typedef int int32_T;
typedef unsigned int uint32_T;
typedef float real32_T;
typedef double real64_T;

/*===========================================================================*
 * Generic type definitions: boolean_T, char_T, byte_T, int_T, uint_T,       *
 *                           real_T, time_T, ulong_T.                        *
 *===========================================================================*/
typedef double real_T;
typedef double time_T;
typedef unsigned char boolean_T;
typedef int int_T;
typedef unsigned int uint_T;
typedef unsigned long ulong_T;
typedef char char_T;
typedef unsigned char uchar_T;
typedef char_T byte_T;

/*=======================================================================*
 * Min and Max:                                                          *
 *   int8_T, int16_T, int32_T     - signed 8, 16, or 32 bit integers     *
 *   uint8_T, uint16_T, uint32_T  - unsigned 8, 16, or 32 bit integers   *
 *=======================================================================*/
#define MAX_int8_T                     ((int8_T)(127))
#define MIN_int8_T                     ((int8_T)(-128))
#define MAX_uint8_T                    ((uint8_T)(255U))
#define MAX_int16_T                    ((int16_T)(32767))
#define MIN_int16_T                    ((int16_T)(-32768))
#define MAX_uint16_T                   ((uint16_T)(65535U))
#define MAX_int32_T                    ((int32_T)(2147483647))
#define MIN_int32_T                    ((int32_T)(-2147483647-1))
#define MAX_uint32_T                   ((uint32_T)(0xFFFFFFFFU))

/* Block D-Work pointer type */
typedef void * pointer_T;

#endif                                 /* RTWTYPES_H */
