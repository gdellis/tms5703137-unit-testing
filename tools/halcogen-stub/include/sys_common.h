/* Stand-in for HALCoGen's sys_common.h: just the integer typedefs the stub SCI API
 * and target/unity_target_io.c need. See ../README.md. */
#ifndef __SYS_COMMON_H__
#define __SYS_COMMON_H__

#include <stdint.h>

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   sint8;
typedef int16_t  sint16;
typedef int32_t  sint32;
typedef int64_t  sint64;
typedef uint8_t  boolean;

#ifndef TRUE
#define TRUE  (1U)
#endif
#ifndef FALSE
#define FALSE (0U)
#endif

#endif /* __SYS_COMMON_H__ */
