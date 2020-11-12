#include <stdlib.h>
#include "qe.h"

#if 0
/* add empty pointers at the end of init and cleanup sections */
static int (*__initcall_end)(void) __init_call = NULL;

static void (*__exitcall_end)(void) __exit_call = NULL;
#else
int modules_end(void)
{
    return 0;
}

qe_module_init(modules_end);
#endif