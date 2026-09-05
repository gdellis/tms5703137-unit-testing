/* Stand-in for HALCoGen's sys_core.h: only the entry point target/unity_target_io.c
 * calls. The real one declares the whole core-init family implemented in
 * sys_core.asm. See ../README.md. */
#ifndef __SYS_CORE_H__
#define __SYS_CORE_H__

#include "sys_common.h"

/** Enable the vector floating point unit. */
void _coreEnableVfp_(void);

#endif /* __SYS_CORE_H__ */
