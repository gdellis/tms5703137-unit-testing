/**
 * @file unity_config.h
 * @brief Unity configuration for running the test binaries on the TMS570LS3137.
 *
 * Picked up because target/CMakeLists.txt defines UNITY_INCLUDE_CONFIG_H and puts this
 * directory on Unity's include path. The host presets never use it. Reference for
 * every option: Unity's docs/UnityConfigurationGuide.md.
 */
#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

/* Cortex-R4F with the TI compiler: ILP32. Unity would otherwise guess from limits.h,
 * which is right here too, but being explicit is the point of this file. */
#define UNITY_INT_WIDTH     32
#define UNITY_LONG_WIDTH    32
#define UNITY_POINTER_WIDTH 32

/* Output: one byte at a time over an SCI, implemented in unity_target_io.c.
 * The *_HEADER_DECLARATION twins make Unity emit the prototypes. */
#define UNITY_OUTPUT_CHAR(c)                     unity_target_putc(c)
#define UNITY_OUTPUT_CHAR_HEADER_DECLARATION     unity_target_putc(int c)
#define UNITY_OUTPUT_FLUSH()                     unity_target_flush()
#define UNITY_OUTPUT_FLUSH_HEADER_DECLARATION    unity_target_flush(void)
#define UNITY_OUTPUT_START()                     unity_target_start()
#define UNITY_OUTPUT_START_HEADER_DECLARATION    unity_target_start(void)
#define UNITY_OUTPUT_COMPLETE()                  unity_target_complete()
#define UNITY_OUTPUT_COMPLETE_HEADER_DECLARATION unity_target_complete(void)

/* Floating-point asserts stay enabled. HALCoGen's _c_int00 does NOT turn the VFP on,
 * so unity_target_io.c calls _coreEnableVfp_() before the first test runs (see
 * TMS570_ENABLE_VFP in target/CMakeLists.txt). On a part with no FPU, define
 * UNITY_EXCLUDE_FLOAT here and turn TMS570_ENABLE_VFP off: the float asserts then
 * become compile errors rather than undefined-instruction traps. */

/* No ANSI colour over the serial link (Unity default). */

#endif /* UNITY_CONFIG_H */
