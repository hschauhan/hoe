/*
 * cscope mode for QEmacs.
 * Copyright (c) 2020 Himanshu Chauhan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "qe.h"
#include <pwd.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

#define PRETTY_WRITE 1

typedef struct CscopeOutput {
    char file[1024];
    int line;
    char sym[256];
    char context[1024];
} CscopeOutput;

typedef struct CscopeMark {
    EditBuffer *b;
    int offset;
} CscopeMark;

typedef struct CscopeMarkStack {
#define CSCOPE_STACK_SZ		1024
    int index;
    CscopeMark stack[1024];
} CscopeMarkStack;

typedef struct CscopeState {
    EditState *os;
    int op;
    char *sym;
    char *symdir;
    CscopeOutput *out;
    int entries;
    CscopeMarkStack cstack;
} CscopeState;

int split_horizontal = 1;

ModeDef cscope_mode;

CscopeState cs;

#define OUTBUF_WIN_SZ	1024

void do_load_at_line(EditState *s, const char *filename, int line);

static int cscope_push_mark(EditBuffer *b, int offset)
{
    if (cs.cstack.index >= CSCOPE_STACK_SZ)
        return -1;

    cs.cstack.index++;
    cs.cstack.stack[cs.cstack.index].b = b;
    cs.cstack.stack[cs.cstack.index].offset = offset;

    return 0;
}

static int cscope_pop_mark(CscopeMark *m)
{
    if (cs.cstack.index < 0)
        return -1;

    m->b = cs.cstack.stack[cs.cstack.index].b;
    m->offset = cs.cstack.stack[cs.cstack.index].offset;
    cs.cstack.index--;

    return 0;
}

static void cscope_free_previous_allocs(void)
{
    if (cs.out) {
        free(cs.out);
        cs.out = NULL;
    }
    if (cs.sym) {
        free(cs.sym);
        cs.sym = NULL;
    }
}

static char cscope_symbol_file_exists(char *dir)
{
    char cscope_file[2048];
    struct stat st;

    snprintf(cscope_file, sizeof(cscope_file), "%s/cscope.out", dir);

    if (stat(cscope_file, &st) < 0) {
        goto out;
    }

    if ((st.st_mode & S_IFMT) != S_IFREG) {
        goto out;
    }

    return 1;

out:
    return 0;
}

char *cscope_get_home_dir(void)
{
    char *homedir = NULL;

    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }

    return homedir;
}

static char *cscope_search_symbol_file(char *dir)
{
    char *nd;
    //char *homedir = get_home_dir();

    if (!cscope_symbol_file_exists(dir)) {
        nd = dirname(dir);
        if (strlen(nd) == strlen("/"))
            return NULL;
        return cscope_search_symbol_file(nd);
    }

    return dir;
}

static void cscope_select_file(EditState *s)
{
    int index;
    char fpath[2048];

    index = list_get_pos(s);
    if (index < 0 || index >= cs.entries)
        return;

    put_status(s, "Save %d offset in %s", s->offset, cs.os->b->name);
    if (cscope_push_mark(cs.os->b, cs.os->offset)) {
        put_status(s, "Cscope stack full!");
    }

    snprintf(fpath, sizeof(fpath), "%s/%s", cs.symdir, cs.out[index].file);
    do_load_at_line(cs.os, fpath, cs.out[index].line);
}

int cscope_query(char *symdir, int opc, char *sym, char **response, int *len)
{
    char cs_command[2048];
    FILE *co;
    int rc;
    int tlen, nr_reads = 1;
    char *outbuf = NULL, *ob;

    snprintf(cs_command, sizeof(cs_command),
             "cscope -p8 -d -f %s/cscope.out -L%d %s", symdir, opc, sym);

    outbuf = malloc(OUTBUF_WIN_SZ);
    if (outbuf == NULL)
        return -1;

    memset(outbuf, 0, OUTBUF_WIN_SZ);

    co = popen(cs_command, "r");
    if (co == NULL) {
        free(outbuf);
        return -1;
    }

    tlen = 0;
    for(;;) {
        rc = fread(outbuf+tlen, 1, OUTBUF_WIN_SZ, co);
        if (rc == 0 || rc < OUTBUF_WIN_SZ) {
            if (ferror(co)) {
                free(outbuf);
                pclose(co);
                return -1;
            }

            if (feof(co)) {
                tlen += rc;
                goto out;
            }
        }

        tlen += rc;
        nr_reads++;
        ob = realloc(outbuf, (nr_reads * OUTBUF_WIN_SZ));
        if (ob == NULL) {
            free(outbuf);
            pclose(co);
            return -1;
        }
        memset(ob + ((nr_reads-1) * OUTBUF_WIN_SZ), 0, OUTBUF_WIN_SZ);
        outbuf = ob;
    }

out:
    *response = outbuf;
    *len = tlen;
    pclose(co);

    if (tlen == 0)
        return -1;

    return 0;
}

void cscope_parse_line(char *line, CscopeOutput *out)
{
    int state = 0;
    int i = 0;
    char lstr[8];

    for (;;) {
        switch(state) {
        case 0: /* file name with relative path */
            while (*line != ' ') {
                if (i >= sizeof(out->file)-1) {
                    line++;
                    continue;
                }
                out->file[i] = *line;
                line++;
                i++;
            }
            out->file[i] = '\0';
            state++;
            line++;
            break;

        case 1: /* symbol scope */
            i = 0;
            while (*line != ' ') {
                if (i >= sizeof(out->sym)-1) {
                    line++;
                    continue;
                }
                out->sym[i] = *line;
                line++;
                i++;
            }
            out->sym[i] = '\0';
            state++;
            line++;
            break;

        case 2: /* line number */
            i = 0;
            memset(lstr, 0, sizeof(lstr));
            while (*line != ' ') {
                if (i >= 7) {
                    line++;
                    continue;
                }
                lstr[i] = *line;
                i++;
                line++;
            }
            lstr[7] = '\0';
            out->line = atoi(lstr);
            state++;
            line++;
            break;

        case 3: /* context or preview of the line */
            i = 0;
            while (*line != '\n' ||
                   *line != '\0' ||
                   *line != 0) {
                       if (i >= sizeof(out->context)-1) {
                           out->context[i-1] = '\0';
                           goto out;
                       }
                       out->context[i] = *line;
                       line++;
                       i++;
            }
            out->context[i] = '\0';
            state++;
            goto out;
            break;

        default:
            break;
        }
    }

 out:
    return;
}

CscopeOutput *cscope_parse_output(char *output, int nr_entries)
{
    int i = 0;
    char *tok;
    CscopeOutput *out = malloc(nr_entries * sizeof(CscopeOutput));
    if (out == NULL)
        return NULL;

    tok = strtok(output, "\n");
    while (tok != NULL) {
        cscope_parse_line(tok, &out[i]);
        i++;
        tok = strtok(NULL, "\n");
    }

    return out;
}

/* colorization states */
enum {
    CS_FILE = 1,
    CS_LINE,
    CS_SYM,
    CS_CONTEXT,
};

void cscope_colorize_line(unsigned int *buf, int len,
                          int *colorize_state_ptr, int state_only)
{
    int state = CS_FILE;
    unsigned int *p, *p1;
    int delim = ' ';

    if (*buf == '\n')
        return;

    p1 = p = buf;

    for (;;) {
        while (*++p == delim) continue;
        p1 = p;
        while (*p != delim) {
            p++;
        }
        switch (state) {
        case CS_FILE:
            set_color(p1, p - p1, QE_STYLE_FUNCTION);
            state++;
            break;

        case CS_SYM:
            set_color(p1, p - p1, QE_STYLE_TYPE);
            state++;
            delim = '\n';
            break;

        case CS_LINE:
            set_color(p1, p - p1, QE_STYLE_STRING);
            state++;
            break;

        case CS_CONTEXT:
            set_color(p1, p - p1, QE_STYLE_PREPROCESS);
            state++;
            goto out;
            break;
        }
    }

out:
    return;
}

/* show a list of buffers */
void cscope_query_and_show(EditState *s)
{
    QEmacsState *qs = s->qe_state;
    EditBuffer *b;
    EditState *e;
    int x, y, rlen, ln, cn;
    char *cs_resp;
    char fpath[2048];

    if (cscope_query(cs.symdir, cs.op, cs.sym, &cs_resp, &rlen) < 0) {
        put_status(s, "cscope query failed");
        return;
    }

    if ((b = eb_find("*cscope*")) == NULL) {
        b = eb_new("*cscope*", BF_READONLY | BF_SYSTEM);
        if (b == NULL)
            return;
    } else
        /* if found a previous buffer, clear it up */
        eb_delete(b, 0, b->total_size);

    /* write the current cscope output */
    eb_write(b, 0, (unsigned char *)cs_resp, rlen);
    eb_get_pos(b, &ln, &cn, b->total_size);
    cs.entries = ln;
    cs.out = cscope_parse_output(cs_resp, ln);
    (void)cn;

#if PRETTY_WRITE
    y = 0; /* offset in buffer */
    eb_delete(b, 0, b->total_size);
    for (cn = 0; cn < ln; cn++) {
        x = snprintf(fpath, sizeof(fpath),
                     "%-32s [%d] %-24s %s\n",
                     cs.out[cn].file, cs.out[cn].line,
                     cs.out[cn].sym, cs.out[cn].context);
        eb_write(b, y, (unsigned char *)fpath, x);
        y += x;
        memset(fpath, 0, sizeof(fpath));
    }
#endif

    if (ln > 1) {
        if (!split_horizontal) {
            x = (s->x2 + s->x1) / 2;
            e = edit_new(b, x, s->y1, s->x2 - x,
                         s->y2 - s->y1, WF_MODELINE);

            s->x2 = x;
            s->flags |= WF_RSEPARATOR;
        } else {
            y = (s->y2 + s->y1) / 2;
            e = edit_new(b, s->x1, y,
                         s->x2 - s->x1, s->y2 - y,
                         WF_MODELINE | (s->flags & WF_RSEPARATOR));
            s->y2 = y;
        }

        do_set_mode(e, &cscope_mode, NULL);

        qs->active_window = e;
        do_refresh(e);
    } else {
        put_status(s, "Save %d offset in %s", s->offset, cs.os->b->name);
        cscope_push_mark(s->b, s->offset);
        snprintf(fpath, sizeof(fpath), "%s/%s", cs.symdir, cs.out[0].file);
        do_load_at_line(s, fpath, cs.out[0].line);
    }
}

static void cscope_query_symbol(void *opaque, char *reply)
{
    if (reply && strlen(reply) != 0) {
        if (cs.sym) free(cs.sym);
        cs.sym = strdup(reply);
        free(reply);
    } else if (reply == NULL)
        return;

    if (cs.sym == NULL)
        return;

    cscope_query_and_show(cs.os);
}

static void do_cscope_operation(EditState *s, int op)
{
    char status[512];
    char suggestion[256];
    cs.op = op;
    cs.os = s;
    char current_dir[1024], *c;
    char *cdir = getcwd(current_dir, sizeof(current_dir));

    cscope_free_previous_allocs();

    if (cs.symdir == NULL) {
        c = cscope_search_symbol_file(cdir);
        if (c)
            cs.symdir = strdup(c);
    }

    if (cs.symdir == NULL) {
        put_status(s, "No symbol file located or provided. Please provide symbol file with C-X-RET s");
        return;
    } else {
        put_status(s, "Symbol file at: %s", cs.symdir);
    }

    cs.sym = do_read_word_at_offset(s);
    if (cs.sym == NULL || (cs.sym && strlen(cs.sym) == 0)) {
        memset(suggestion, 0, sizeof(suggestion));
        if (cs.sym) free(cs.sym);
        cs.sym = NULL;
    } else {
        snprintf(suggestion, sizeof(suggestion), "[default %s]", cs.sym);
    }

    qe_ungrab_keys();
    switch(op) {
    case 0:
        snprintf(status, sizeof(status), "Find symbol %s: ", suggestion);
        break;
    case 1:
        snprintf(status, sizeof(status), "Find global definition %s: ", suggestion);
        break;
    case 2:
        snprintf(status, sizeof(status), "Find functions called by this function %s: ", suggestion);
        break;
    case 3:
        snprintf(status, sizeof(status), "Find functions calling this function %s: ", suggestion);
        break;
    case 4:
        snprintf(status, sizeof(status), "Find text string %s: ", suggestion);
        break;
    case 5:
        put_status(s, "Change text string not supported");
        return;
    case 6:
        put_status(s, "Find egrep pattern not supported ");
        return;
    case 7:
        snprintf(status, sizeof(status), "Find file %s: ", suggestion);
        break;
    case 8:
        snprintf(status, sizeof(status), "Find #including file %s: ", suggestion);
        break;
    case 9:
        snprintf(status, sizeof(status), "Find assignments to symbol %s: ", suggestion);
        break;
    }

    minibuffer_edit(NULL, status, NULL, NULL,
                    cscope_query_symbol, (void *)s);
}

static void do_cscope_pop_mark(EditState *s)
{
    CscopeMark csm;
    if (cscope_pop_mark(&csm)) {
        put_status(s, "Cscope stack is empty");
        return;
    }

    put_status(s, "Pop %d offset in %s", csm.offset, csm.b->name);

    cs.os->offset = csm.offset;
    switch_to_buffer(cs.os, csm.b);
}

static void do_query_symbol_directory(void *opaque, char *reply)
{
    EditState *s = (EditState *)opaque;
    const char *homedir;
    char *or = reply;
    struct stat st;
    cs.symdir = malloc(1024);

    if (cs.symdir == NULL) {
        put_status(s, "No memory!");
        return;
    }

    if (reply == NULL)
        return;

    if (*reply == '~') {
        homedir = cscope_get_home_dir();

        reply++;

        /* since ~file is also valid check if / is given by user
         * and skip it.
         */
        if (*reply == '/') reply++;
        snprintf(cs.symdir, 1024, "%s/%s", homedir, reply);
    } else if (*reply == '/') {
        strncpy(cs.symdir, reply, 1023);
        cs.symdir[1023] = '\0';
    } else {
        put_status(s, "Please provide absolute path.");
        goto out;
    }

    if (stat(cs.symdir, &st) < 0) {
        if (errno == ENOENT) {
            put_status(s, "Symbol directory doesn't exist");
        } else {
            put_status(s, "Unknown error in checking symbol directory");
        }
        goto out;
    }

    if ((st.st_mode & S_IFMT) != S_IFDIR) {
        put_status(s, "Symbol path is not a directory");
        goto out;
    }

    if (!cscope_symbol_file_exists(cs.symdir)) {
        put_status(s, "No symbol file in %s directory", cs.symdir);
        goto out;
    }

    free(or);

    put_status(s, "Cscope symbol file at: %s", cs.symdir);

    return;

out:
    free(cs.symdir);
    cs.symdir = NULL;
    free(or);
}

static void do_cscope_set_symbol_directory(EditState *s)
{
    char current_dir[1024];
    char *cdir = getcwd(current_dir, sizeof(current_dir));

    qe_ungrab_keys();
    minibuffer_edit(cdir, "Symbol File Directory: ",
                    NULL, file_completion,
                    do_query_symbol_directory, (void *)s);
}

/* specific bufed commands */
static CmdDef cscope_mode_commands[] = {
    CMD0( KEY_RET, ' ', "cscope-select", cscope_select_file)
    CMD1( KEY_CTRL('g'), KEY_NONE, "delete-window", do_delete_window, 0)
    CMD_DEF_END,
};

static CmdDef cscope_global_commands[] = {
    CMD0( KEY_CTRLXRET('s'), KEY_NONE, "cscope-set-symbol-directory", do_cscope_set_symbol_directory)
    CMD1( KEY_F2, KEY_NONE, "cscope-find-symbol", do_cscope_operation, 0)
    CMD1( KEY_F3, KEY_NONE, "cscope-find-global-definition",
         do_cscope_operation, 1)
    CMD1( KEY_F4, KEY_NONE, "cscope-find-function-called-by-this-function",
         do_cscope_operation, 2)
    CMD1( KEY_F5, KEY_NONE, "cscope-find-function-calling-this-function",
         do_cscope_operation, 3)
    CMD1( KEY_F6, KEY_NONE, "cscope-find-string",
         do_cscope_operation, 4)
    CMD1( KEY_F7, KEY_NONE, "cscope-find-file",
         do_cscope_operation, 7)
    CMD1( KEY_F8, KEY_NONE, "cscope-find-#including-file",
         do_cscope_operation, 8)
    CMD1( KEY_F9, KEY_NONE, "cscope-find-assignment",
         do_cscope_operation, 9)
    CMD0( KEY_NONE, KEY_NONE, "cscope-pop-mark",
         do_cscope_pop_mark)
    CMD_DEF_END,
};

static int cscope_mode_init(EditState *s, ModeSavedData *saved_data)
{
    list_mode.mode_init(s, saved_data);
    set_colorize_func(s, cscope_colorize_line);
    return 0;
}

static void cscope_mode_close(EditState *s)
{
    list_mode.mode_close(s);
}

static int cscope_mode_probe(ModeProbeData *p)
{
    const char *r;

    /* currently, only use the file extension */
    r = strrchr((char *)p->filename, '.');
    if (r) {
        r++;
        if (!strcasecmp(r, "c")   ||
            !strcasecmp(r, "h")   ||
            !strcasecmp(r, "cpp"))
            return 100;
    }
    return 0;
}

static int cscope_init(void)
{
    cs.cstack.index = -1;

    /* inherit from list mode */
    memcpy(&cscope_mode, &list_mode, sizeof(ModeDef));
    cscope_mode.name = "cscope";
    cscope_mode.instance_size = sizeof(CscopeState);
    cscope_mode.mode_init = cscope_mode_init;
    cscope_mode.mode_probe = cscope_mode_probe;
    cscope_mode.mode_close = cscope_mode_close;

    /* first register mode */
    qe_register_mode(&cscope_mode);

    qe_register_cmd_table(cscope_mode_commands, "cscope");
    qe_register_cmd_table(cscope_global_commands, NULL);

    return 0;
}

qe_module_init(cscope_init);
