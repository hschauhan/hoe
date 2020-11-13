/*
 * LaTeX mode for QEmacs.
 * Copyright (c) 2003 Martin Hedenfalk <mhe@home.se>
 * Based on c-mode by Fabrice Bellard
 * Requires the shell mode
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

#define MAX_BUF_SIZE    512

static ModeDef latex_mode;

/* TODO: add state handling to allow colorization of elements longer
 * than one line (eg, multi-line functions and strings)
 */
static void latex_colorize_line(unsigned int *buf, int len,
                     int *colorize_state_ptr, int state_only)
{
    int c, state;
    unsigned int *p, *p_start;

    state = *colorize_state_ptr;
    p = buf;
    p_start = p;

    for(;;) {
        p_start = p;
        c = *p;
        switch(c) {
        case '\n':
            goto the_end;
        case '`':
            p++;
            /* a ``string'' */
            if(*p == '`') {
                while(1) {
                    p++;
                    if(*p == '\n' || (*p == '\'' && *(p+1) == '\''))
                        break;
                }
                if(*p == '\'' && *++p == '\'')
                    p++;
                set_color(p_start, p - p_start, QE_STYLE_STRING);
            }
            break;
        case '\\':
            p++;
            /* \function[keyword]{variable} */
            if(*p == '\'' || *p == '\"' || *p == '~' || *p == '%' || *p == '\\')
                p++;
            else {
                while(*p != '{' && *p != '[' && *p != '\n' && *p != ' ' && *p != '\\')
                    p++;
            }
            set_color(p_start, p - p_start, QE_STYLE_FUNCTION);
            while(*p == ' ' || *p == '\t')
                /* skip space */
                p++;
            while(*p == '{' || *p == '[') {
                if(*p++ == '[') {
                    /* handle [keyword] */
                    p_start = p;
                    while(*p != ']' && *p != '\n')
                        p++;
                    set_color(p_start, p - p_start, QE_STYLE_KEYWORD);
                    if(*p == ']')
                        p++;
                } else {
                    int braces = 0;
                    /* handle {variable} */
                    p_start = p;
                    while(*p != '\n') {
                        if(*p == '{')
                        braces++;
                        else if(*p == '}')
                            if(braces-- == 0)
                            break;
                        p++;
                    }
                    set_color(p_start, p - p_start, QE_STYLE_VARIABLE);
                    if(*p == '}')
                        p++;
                }
                while(*p == ' ' || *p == '\t')
                    /* skip space */
                    p++;
            }
            break;
        case '%':
            p++;
            /* line comment */
            while (*p != '\n')
                p++;
            set_color(p_start, p - p_start, QE_STYLE_COMMENT);
            break;
        default:
            p++;
            break;
        }
    }
 the_end:
    *colorize_state_ptr = state;
}

static int latex_mode_probe(ModeProbeData *p)
{
    const char *r;

    /* currently, only use the file extension */
    r = strrchr((char *)p->filename, '.');
    if (r) {
        r++;
        if (strcasecmp(r, "tex") == 0)
            return 100;
    }
    return 0;
}

static int latex_mode_init(EditState *s, ModeSavedData *saved_data)
{
    int ret;
    ret = text_mode_init(s, saved_data);
    if (ret)
        return ret;
    set_colorize_func(s, latex_colorize_line);
    return ret;
}

static int latex_init(void)
{
    /* LaTeX mode is almost like the text mode, so we copy and patch it */
    memcpy(&latex_mode, &text_mode, sizeof(ModeDef));
    latex_mode.name = "LaTeX";
    latex_mode.mode_probe = latex_mode_probe;
    latex_mode.mode_init = latex_mode_init;

    qe_register_mode(&latex_mode);

    return 0;
}

qe_module_init(latex_init);
