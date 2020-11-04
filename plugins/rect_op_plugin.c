/*
 * Rectangle operation plugin
 */
#include "qe.h"

/* same as emacs' "string-insert-rectangle" */
static void do_string_insert_rect(void *opaque, char *reply)
{
    int fline_num, fcol_num, tline_num, tcol_num, i, nr_lines;
    EditState *s = (EditState *)opaque;
  
    eb_get_pos(s->b, &tline_num, &tcol_num, s->offset);
    eb_get_pos(s->b, &fline_num, &fcol_num, s->b->mark);
    fline_num++;
    tline_num++;
    nr_lines = (tline_num - fline_num);

    for (i = 0; i <= nr_lines; i++) {
        do_goto_line(s, fline_num + i);
        s->offset += fcol_num;
        eb_insert(s->b, s->offset, (u8 *)reply, strlen(reply));
        s->offset += strlen(reply);
    }

    free(reply);  
}

static void query_string_insert_rect(EditState *s)
{
    qe_ungrab_keys();
    minibuffer_edit(NULL, "Insert String: ",
                    NULL, NULL,
                    do_string_insert_rect, (void *)s);
}

static CmdDef rect_op_commands[] = {
    CMD0(KEY_CTRLX('i'), KEY_NONE, "string-insert-rectangle", query_string_insert_rect)
    CMD_DEF_END,
};

static int rect_op_plugin_init(void)
{
    /* commands and default keys */
    qe_register_cmd_table(rect_op_commands, NULL);

    return 0;
}

qe_module_init(rect_op_plugin_init);
