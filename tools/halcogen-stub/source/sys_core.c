/* Stand-in for the part of HALCoGen's sys_core.asm that unity_target_io.c calls.
 * A no-op, like the rest of the stub: these images link but must never be run.
 * See ../README.md. */
#include "sys_core.h"

void _coreEnableVfp_(void)
{
}
