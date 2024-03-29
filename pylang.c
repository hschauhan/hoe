/*
 * Python mode for QEmacs.
 * Copyright (c) 2022, Himanshu Chauhan
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

static const char py_keywords[] = 
	"|def|break|continue|do|else|for|elif|try|except|pass|throw|"
	"if|return|while|with|as|";

/* NOTE: 'var' is added for javascript */
static const char py_types[] = 
"|char|double|float|int|long|unsigned|short|signed|void|var|"
"u8|uint8_t|u16|uint16_t|u32|uint32_t|u64|uint64_t|";

static int get_py_keyword(char *buf, int buf_size, unsigned int **pp)
{
    unsigned int *p, c;
    char *q;

    p = *pp;
    c = *p;
    q = buf;
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') || 
        (c == '_')) {
        do {
            if ((q - buf) < buf_size - 1)
                *q++ = c;
            p++;
            c = *p;
        } while ((c >= 'a' && c <= 'z') ||
                 (c >= 'A' && c <= 'Z') ||
                 (c == '_') ||
                 (c >= '0' && c <= '9'));
    }
    *q = '\0';
    *pp = p;
    return q - buf;
}

/* colorization states */
enum {
    PY_COMMENT = 1,
    PY_STRING,
    PY_STRING_Q,
    PY_IF0,
};

void py_colorize_line(unsigned int *buf, int len, 
		      int *colorize_state_ptr, int state_only)
{
    int c, state, l, type_decl;
    unsigned int *p, *p_start, *p1;
    char kbuf[32];

    state = *colorize_state_ptr;
    p = buf;
    p_start = p;
    type_decl = 0;

    /* if already in a state, go directly in the code parsing it */
    switch(state) {
    case PY_STRING:
    case PY_STRING_Q:
        goto parse_string;
    default:
        break;
    }

    for(;;) {
        p_start = p;
        c = *p;
        switch(c) {
        case '\n':
            goto the_end;
        case '#':
	    /* line comment */
	    while (*p != '\n') 
		    p++;
	    set_color(p_start, p - p_start, QE_STYLE_COMMENT);
            break;
        case '\'':
            state = PY_STRING_Q;
            goto string;
        case '\"':
            /* strings/chars */
            state = PY_STRING;
        string:
            p++;
        parse_string:
            while (*p != '\n') {
                if (*p == '\\') {
                    p++;
                    if (*p == '\n')
                        break;
                    p++;
                } else if ((*p == '\'' && state == PY_STRING_Q) ||
                           (*p == '\"' && state == PY_STRING)) {
                    p++;
                    state = 0;
                    break;
                } else {
                    p++;
                }
            }
            set_color(p_start, p - p_start, QE_STYLE_STRING);
            break;
        case '=':
            p++;
            /* exit type declaration */
            type_decl = 0;
            break;
        default:
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') || 
                (c == '_')) {
                
                l = get_py_keyword(kbuf + 1, sizeof(kbuf) - 2, &p);
                kbuf[0] = '|';
                kbuf[l + 1] = '|';
                kbuf[l + 2] = '\0';
                p1 = p;
                if (strstr(py_keywords, kbuf)) {
                    set_color(p_start, p1 - p_start, QE_STYLE_KEYWORD);
                } else if (strstr(py_types, kbuf)) {
                    /* c type */
                    while (*p == ' ' || *p == '\t')
                        p++;
                    /* if not cast, assume type declaration */
                    if (*p != ')') {
                        type_decl = 1;
                    }
                    set_color(p_start, p1 - p_start, QE_STYLE_TYPE);
                } else {
		    type_decl = 1;

                    if (type_decl) {
                        while (*p == ' ' || *p == '\t')
                            p++;
                        if (*p == '(') {
                            /* function definition case */
                            set_color(p_start, p1 - p_start, QE_STYLE_FUNCTION);
                            type_decl = 1;
                        } else if (p_start == buf) {
                            /* assume type if first column */
                            set_color(p_start, p1 - p_start, QE_STYLE_TYPE);
                        } else {
                            set_color(p_start, p1 - p_start, QE_STYLE_VARIABLE);
                        }
                    }
                }
            } else {
                p++;
            }
            break;
        }
    }
 the_end:
    /* highlight the lines that overstep the margins */
    if (g_highlight_over_margin && len > g_margin_size) {
	    /* clear previous from margin to the end of line */
	    clear_color(buf+g_margin_size, len - g_margin_size);
	    set_color(buf+g_margin_size, len - g_margin_size,
		      QE_STYLE_MARGIN_HIGHLIGHT);
    }

    *colorize_state_ptr = state;
}

#define MAX_BUF_SIZE    512
#define MAX_STACK_SIZE  64

/* gives the position of the first non while space character in
   buf. TABs are counted correctly */
static int find_indent1(EditState *s, unsigned int *buf)
{
    unsigned int *p;
    int pos, c;

    p = buf;
    pos = 0;
    for(;;) {
        c = *p++ & CHAR_MASK;
        if (c == '\t')
            pos += s->tab_size - (pos % s->tab_size);
        else if (c == ' ')
            pos++;
        else
            break;
    }
    return pos;
}

static int find_pos(EditState *s, unsigned int *buf, int size)
{
    int pos, c, i;

    pos = 0;
    for(i=0;i<size;i++) {
        c = buf[i] & CHAR_MASK;
        if (c == '\t')
            pos += s->tab_size - (pos % s->tab_size);
        else
            pos++;
    }
    return pos;
}

enum {
    INDENT_NORM,
    INDENT_FIND_EQ,
};

/* insert n spaces at *offset_ptr. Update offset_ptr to point just
   after. Tabs are inserted if s->indent_tabs_mode is true. */
static void insert_spaces(EditState *s, int *offset_ptr, int i)
{
    int offset, size;
    unsigned char buf1[64];

    offset = *offset_ptr;

    /* insert tabs */
    if (s->indent_tabs_mode) {
        while (i >= s->tab_size) {
            buf1[0] = '\t';
            eb_insert(s->b, offset, buf1, 1);
            offset++;
            i -= s->tab_size;
        }
    }
       
    /* insert needed spaces */
    while (i > 0) {
        size = i;
        if (size > sizeof(buf1))
            size = sizeof(buf1);
        memset(buf1, ' ', size);
        eb_insert(s->b, offset, buf1, size);
        i -= size;
        offset += size;
    }
    *offset_ptr = offset;
}

static void do_py_indent(EditState *s)
{
    int offset, offset1, offset2, offsetl, c, pos, size, line_num, col_num;
    int i, eoi_found, len, pos1, lpos, style, line_num1, state;
    unsigned int buf[MAX_BUF_SIZE], *p;
    unsigned char stack[MAX_STACK_SIZE];
    char buf1[64], *q;
    int stack_ptr;

    (void)stack;

    /* find start of line */
    eb_get_pos(s->b, &line_num, &col_num, s->offset);
    line_num1 = line_num;
    offset = s->offset;
    offset = eb_goto_bol(s->b, offset);
    /* now find previous lines and compute indent */
    pos = 0;
    lpos = -1; /* position of the last instruction start */
    offsetl = offset;
    eoi_found = 0;
    stack_ptr = 0;
    state = INDENT_NORM;
    for(;;) {
        if (offsetl == 0)
            break;
        line_num--;
        eb_prevc(s->b, offsetl, &offsetl);
        offsetl = eb_goto_bol(s->b, offsetl);
        len = get_colorized_line(s, buf, MAX_BUF_SIZE - 1, offsetl, line_num);
        /* store indent position */
        pos1 = find_indent1(s, buf);
        p = buf + len;
        while (p > buf) {
            p--;
            c = *p;
            /* skip strings or comments */
            style = c >> STYLE_SHIFT;
            if (style == QE_STYLE_COMMENT ||
                style == QE_STYLE_STRING ||
                style == QE_STYLE_PREPROCESS)
                continue;
            c = c & CHAR_MASK;
            if (state == INDENT_FIND_EQ) {
                /* special case to search '=' or ; before { to know if
                   we are in data definition */
                if (c == '=') {
                    /* data definition case */
                    pos = lpos;
                    goto end_parse;
                } else if (c == ';') {
                    /* normal instruction case */
                    goto check_instr;
                }
            } else {
                switch(c) {
                case '}':
                    if (stack_ptr >= MAX_STACK_SIZE)
                        return;
                    stack[stack_ptr++] = c;
                    goto check_instr;
                case '{':
                    if (stack_ptr == 0) {
                        if (lpos == -1) {
                            pos = pos1 + s->indent_size;
                            eoi_found = 1;
                            goto end_parse;
                        } else {
                            state = INDENT_FIND_EQ;
                        }
                    } else {
                        /* XXX: syntax check ? */
                        stack_ptr--;
                        goto check_instr;
                    }
                    break;
                case ')':
                case ']':
                    if (stack_ptr >= MAX_STACK_SIZE)
                        return;
                    stack[stack_ptr++] = c;
                    break;
                case '(':
                case '[':
                    if (stack_ptr == 0) {
                        pos = find_pos(s, buf, p - buf) + 1;
                        goto end_parse;
                    } else {
                        /* XXX: syntax check ? */
                        stack_ptr--;
                    }
                    break;
                case ' ':
                case '\t':
                case '\n':
                    break;
                case ';':
                    /* level test needed for 'for(;;)' */
                    if (stack_ptr == 0) {
                        /* ; { or } are found before an instruction */
                    check_instr:
                        if (lpos >= 0) {
                            /* start of instruction already found */
                            pos = lpos;
                            if (!eoi_found)
                                pos += s->indent_size;
                            goto end_parse;
                        }
                        eoi_found = 1;
                    }
                    break;
                case ':':
                    /* a label line is ignored */
                    /* XXX: incorrect */
                    goto prev_line;
                default:
                    if (stack_ptr == 0) {
                        if ((c >> STYLE_SHIFT) == QE_STYLE_KEYWORD) {
                            unsigned int *p1, *p2;
                            /* special case for if/for/while */
                            p1 = p;
                            while (p > buf && 
                                   (p[-1] >> STYLE_SHIFT) == QE_STYLE_KEYWORD)
                                p--;
                            p2 = p;
                            q = buf1;
                            while (q < buf1 + sizeof(buf1) - 1 && p2 <= p1) {
                                *q++ = *p2++ & CHAR_MASK;
                            }
                            *q = '\0';

                            if (!eoi_found && 
                                (!strcmp(buf1, "if") ||
                                 !strcmp(buf1, "for") ||
                                 !strcmp(buf1, "while"))) {
                                pos = pos1 + s->indent_size;
                                goto end_parse;
                            }
                        }
                        lpos = pos1;
                    }
                    break;
                }
            }
        }
    prev_line: ;
    }
 end_parse:
    /* compute special cases which depend on the chars on the current line */
    len = get_colorized_line(s, buf, MAX_BUF_SIZE - 1, offset, line_num1);

    if (stack_ptr == 0) {
	if (!pos && lpos >= 0) {
	    /* start of instruction already found */
	    pos = lpos;
	    if (!eoi_found)
		pos += s->indent_size;
	}
    }

    for(i=0;i<len;i++) {
        c = buf[i]; 
        style = c >> STYLE_SHIFT;
        /* if preprocess, no indent */
        if (style == QE_STYLE_PREPROCESS) {
            pos = 0;
            break;
        }
        /* NOTE: strings & comments are correctly ignored there */
        if (c == '}' || c == ':') {
            pos -= s->indent_size;
            if (pos < 0)
                pos = 0;
            break;
        }
        if (c == '{' && pos == s->indent_size && !eoi_found) {
            pos = 0;
            break;
        }
    }
    
    /* the number of needed spaces is in 'pos' */

    /* suppress leading spaces */
    offset1 = offset;
    for(;;) {
        c = eb_nextc(s->b, offset1, &offset2);
        if (c != ' ' && c != '\t')
            break;
        offset1 = offset2;
    }
    size = offset1 - offset;
    if (size > 0) {
        eb_delete(s->b, offset, size);
        s->offset -= size;
        if (s->offset < offset)
            s->offset = offset;
    }
    /* insert needed spaces */
    insert_spaces(s, &offset, pos);
    s->offset = offset;
}
    
void do_py_indent_region(EditState *s)
{
    int col_num, p1, p2, tmp;

    /* we do it with lines to avoid offset variations during indenting */
    eb_get_pos(s->b, &p1, &col_num, s->offset);
    eb_get_pos(s->b, &p2, &col_num, s->b->mark);

    if (p1 > p2) {
        tmp = p1;
        p1 = p2;
        p2 = tmp;
    }

    for(;p1<=p2;p1++) {
        s->offset = eb_goto_pos(s->b, p1, 0);
        do_py_indent(s);
    }
}

void do_py_electric(EditState *s, int key)
{
    do_char(s, key);
    do_py_indent(s);
}

static int py_mode_probe(ModeProbeData *p)
{
    const char *r;

    /* currently, only use the file extension */
    r = strrchr((char *)p->filename, '.');
    if (r) {
        r++;
        if (!strcasecmp(r, "py"))
            return 100;
    }
    return 0;
}

int py_mode_init(EditState *s, ModeSavedData *saved_data)
{
    int ret;
    ret = text_mode_init(s, saved_data);
    if (ret)
        return ret;
    set_colorize_func(s, py_colorize_line);
    return ret;
}

/* specific python commands */
static CmdDef py_commands[] = {
    CMD0( KEY_CTRLXRET('i'), KEY_NONE, "py-indent-command", do_py_indent)
    CMD0( KEY_NONE, KEY_NONE, "py-indent-region", do_py_indent_region)
    CMD1( ':', KEY_NONE, "py-electric-colon", do_py_electric, ':')
    CMD_DEF_END,
};

static ModeDef py_mode;

int py_init(void)
{
    /* py mode is almost like the text mode, so we copy and patch it */
    memcpy(&py_mode, &text_mode, sizeof(ModeDef));
    py_mode.name = "Python";
    py_mode.mode_probe = py_mode_probe;
    py_mode.mode_init = py_mode_init;

    qe_register_mode(&py_mode);

    qe_register_cmd_table(py_commands, "Python");

    return 0;
}

qe_module_init(py_init);
