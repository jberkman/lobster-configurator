#ifndef LOBSTER_IO_H
#define LOBSTER_IO_H

#include <glib/gmacros.h>
#include <glib/gerror.h>

G_BEGIN_DECLS

typedef gboolean  (*LobsterIOReadFileFunc)  (const char *file, int line_no, char *line, gpointer data, GError **error);
typedef char     *(*LobsterIOWriteFileFunc) (const char *file, int line_no, char *line, gpointer data, GError **error);

G_END_DECLS

G_BEGIN_DECLS

gboolean lobster_io_read_file      (const char *file, LobsterIOReadFileFunc func, gpointer data, GError **error);
gboolean lobster_io_overwrite_file (const char *file, LobsterIOWriteFileFunc func, gpointer data, GError **error);

G_END_DECLS

#endif /* LOBSTER_IO_H */
