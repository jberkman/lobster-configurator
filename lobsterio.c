#include "config.h"

#include "lobsterio.h"

#include <glib.h>
#include <stdio.h>

static gboolean
read_by_line (const char *file, LobsterIOReadFileFunc func, gpointer data, GError **error)
{
    char *contents;
    char *line_start;
    char *line_end;
    int line_no;
    GError *our_error = NULL;

    if (!g_file_get_contents (file, &contents, NULL, &our_error)) {
        if (g_error_matches (our_error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            g_error_free (our_error);
            return TRUE;
        }
        g_propagate_error (error, our_error);
        return FALSE;
    }

    for (line_start = line_end = contents, line_no = 1; *line_end; line_end++) {
        if (*line_end == '\n') {
            *line_end = '\0';
            if (!func (file, line_no, line_start, data, error)) {
                g_free (contents);
                return FALSE;
            }
            ++line_no;
            line_start = line_end + 1;
        }
    }
    if (!func (file, line_no, line_start, data, error)) {
        g_free (contents);
        return FALSE;
    }

    g_free (contents);
    return TRUE;
}

gboolean
lobster_io_read_file (const char *file, LobsterIOReadFileFunc func, gpointer data, GError **error)
{
    return read_by_line (file, func, data, error);
}

typedef struct {
    LobsterIOWriteFileFunc func;
    gpointer               data;
    GString               *buffer;
    gboolean               last_was_blank;
} WriteData;

static gboolean
overwrite_line (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    WriteData *wd = data;
    char *new_line;

    new_line = wd->func (file, line_no, line, wd->data, error);
    if (!new_line) {
        return FALSE;
    }
    if (*new_line || !wd->last_was_blank) {
        g_string_append (wd->buffer, new_line);
        g_string_append (wd->buffer, "\n");
        if (new_line != line) {
            g_free (new_line);
        }
    }
    wd->last_was_blank = !*new_line;
    return TRUE;
}

gboolean
lobster_io_overwrite_file (const char *file, LobsterIOWriteFileFunc func, gpointer data, GError **error)
{
    WriteData wd;
    gboolean ret = FALSE;

    wd.func = func;
    wd.data = data;
    wd.buffer = g_string_new (NULL);
    wd.last_was_blank = FALSE;

    if (!lobster_io_read_file (file, overwrite_line, &wd, error)) {
        goto free_buffer;
    }

    /* one last line to let writers write any extra data that may
     * not have been included */
    if (overwrite_line (file, -1, NULL, &wd, error)) {
        ret = g_file_set_contents (file, wd.buffer->str, -1, error);
        fprintf (stderr, "NEW CONTENTS OF %s:\n\n%s\n", file, wd.buffer->str);
    }

free_buffer:
    g_string_free (wd.buffer, TRUE);
    return ret;
}
