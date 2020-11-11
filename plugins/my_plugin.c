/*
 * Simple plugin example
 */
#include "qe.h"

/* insert 'hello' at the current cursor position */
static void insert_hello(EditState *s)
{
    const char *m = "Hello word";
    eb_insert(s->b, s->offset, (unsigned char *)m, strlen(m));
    s->offset += strlen(m);
}

static CmdDef my_commands[] = {
    CMD0( KEY_NONE, KEY_NONE, "insert-hello", insert_hello)
    CMD_DEF_END,
};

static int my_plugin_init(void)
{
    /* commands and default keys */
    qe_register_cmd_table(my_commands, NULL);

    return 0;
}

qe_module_init(my_plugin_init);
