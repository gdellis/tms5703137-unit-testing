/* HALCoGen always generates a sys_main.c with main(). The build must exclude it - the
 * generated Unity runner supplies main(). If the exclusion in target/CMakeLists.txt
 * ever regresses, this file makes the link fail with a duplicate symbol. */
#include "sys_common.h"

int main(void)
{
    return 0;
}
