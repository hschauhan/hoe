/*
 * C mode for QEmacs.
 * Copyright (c) 2001, 2002 Fabrice Bellard.
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

int is_index_line(unsigned int *p)
{
    if (*p == 'I' || *p == 'i') {
        if (p[1] == 'n' &&
            p[2] == 'd' &&
            p[3] == 'e' &&
            p[4] == 'x')
        return 1;
    }

    return 0;
}

int is_file_meta(unsigned int *p)
{
    if ((*p == '-' && p[1] == '-' && p[2] == '-' && p[3] == ' ' && (p[4] == 'a' || p[4] == '/')) ||
        (*p == '+' && p[1] == '+' && p[2] == '+' && p[3] == ' ' && (p[4] == 'b' || p[4] == '/')))
        return 1;
    return 0;
}

int is_addition(unsigned int *p)
{
    if (*p == '+')
        return 1;

    return 0;
}

int is_deletion(unsigned int *p)
{
    if (*p == '-')
        return 1;

    return 0;
}

int is_hunk(unsigned int *p)
{
    if (*p == '@' && p[1] == '@' && p[2] == ' ')
        return 1;

    return 0;
}

void patch_colorize_line(unsigned int *buf, int len, 
                         int *colorize_state_ptr, int state_only)
{
    unsigned int *p;
    int state;

    p = buf;

    state = *colorize_state_ptr;

    if (is_index_line(p)) {
        set_color(buf, len, QE_STYLE_PATCH_INDEX);
        goto the_end;
    }
    
    if (is_file_meta(p)) {
        set_color(buf, len, QE_STYLE_PATCH_FILE_META);
        goto the_end;
    }

    if (is_addition(p)) {
        set_color(buf, len, QE_STYLE_PATCH_ADDITION);
        goto the_end;
    }

    if (is_deletion(p)) {
        set_color(buf, len, QE_STYLE_PATCH_DELETION);
        goto the_end;
    }

    if (is_hunk(p)) {
        set_color(buf, len, QE_STYLE_PATCH_HUNK);
        goto the_end;
    }

the_end:
    *colorize_state_ptr = state;
}

static int patch_mode_probe(ModeProbeData *p)
{
    const char *r;

    /* currently, only use the file extension */
    r = strrchr((char *)p->filename, '.');
    if (r) {
        r++;
        if (!strcasecmp(r, "diff") ||
            !strcasecmp(r, "patch"))
            return 100;
    }
    return 0;
}

int patch_mode_init(EditState *s, ModeSavedData *saved_data)
{
    int ret;
    ret = text_mode_init(s, saved_data);
    if (ret)
        return ret;
    set_colorize_func(s, patch_colorize_line);
    return ret;
}

static ModeDef patch_mode;

int patch_init(void)
{
    /* c mode is almost like the text mode, so we copy and patch it */
    memcpy(&patch_mode, &text_mode, sizeof(ModeDef));
    patch_mode.name = "Patch";
    patch_mode.mode_probe = patch_mode_probe;
    patch_mode.mode_init = patch_mode_init;

    qe_register_mode(&patch_mode);

    return 0;
}

qe_module_init(patch_init);
