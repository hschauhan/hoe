/*
 * Rectangle operation plugin
 */
#include "qe.h"

static void do_kill_rect(EditState *s)
{
    int fline_num, fcol_num, tline_num, tcol_num, i, nr_lines, kill_len;
    char status[128];

    eb_get_pos(s->b, &tline_num, &tcol_num, s->offset);
    eb_get_pos(s->b, &fline_num, &fcol_num, s->b->mark);
    fline_num++;
    tline_num++;
    nr_lines = (tline_num - fline_num);
    kill_len = (tcol_num - fcol_num);
    nr_lines++;

    for (i = 0; i < nr_lines; i++) {
        do_goto_line(s, fline_num + i);
        s->offset += fcol_num;
        eb_delete(s->b, s->offset, kill_len);
        s->offset -= kill_len;
    }

    snprintf(status, sizeof(status), "Killed %d chars from %d lines", (kill_len * nr_lines), nr_lines);
    put_status(s, status);
}

/* same as emacs' "string-insert-rectangle" */
static void do_string_insert_rect(void *opaque, char *reply)
{
    int fline_num, fcol_num, tline_num, tcol_num, i, nr_lines, ilen;
    EditState *s = (EditState *)opaque;
    char status[128];

    eb_get_pos(s->b, &tline_num, &tcol_num, s->offset);
    eb_get_pos(s->b, &fline_num, &fcol_num, s->b->mark);
    fline_num++;
    tline_num++;
    nr_lines = (tline_num - fline_num);
    nr_lines++;
    ilen = strlen(reply);

    for (i = 0; i < nr_lines; i++) {
        do_goto_line(s, fline_num + i);
        s->offset += fcol_num;
        eb_insert(s->b, s->offset, (u8 *)reply, ilen);
        s->offset += strlen(reply);
    }

    snprintf(status, sizeof(status), "Inserted %d characters from line %d to line %d", (ilen * nr_lines), fline_num, tline_num);
    put_status(s, status);

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
    CMD0(KEY_CTRLX('k'), KEY_NONE, "string-kill-rectangle", do_kill_rect)
    CMD_DEF_END,
};

static int rect_op_plugin_init(void)
{
    /* commands and default keys */
    qe_register_cmd_table(rect_op_commands, NULL);

    return 0;
}

qe_module_init(rect_op_plugin_init);
