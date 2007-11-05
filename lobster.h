#ifndef LOBSTER_H
#define LOBSTER_H

#include <glib/gmacros.h>

G_BEGIN_DECLS

typedef struct _LobsterSystem LobsterSystem;
typedef struct _LobsterInterface LobsterInterface;

G_END_DECLS

#include <glib/ghash.h>
#include <glib/gerror.h>
#include <glib/gmain.h>
#include <gtk/gtkwidget.h>

G_BEGIN_DECLS

struct _LobsterSystem {
    GtkWidget  *dialog;
    GList      *interfaces;
    char       *dns_servers;
    char       *router;

    int ignore_edits;

    gboolean    use_nm;
    gboolean    dirty;
};

struct _LobsterInterface {
    char     *interface;
    char     *address;
    char     *subnet;

    gboolean  enabled;
    gboolean  dhcp;

    gboolean  dirty;
};

extern LobsterSystem lobster;

void     lobster_show_error (const char *doing, GError *error);

void     lobster_ignore_edits (void);
void     lobster_accept_edits (void);

gboolean lobster_system_load (GError **error);
gboolean lobster_system_save (GError **error);
void     lobster_system_display (void);

void     lobster_system_dirty    (void);
void     lobster_interface_dirty (void);
gboolean lobster_is_dirty        (void);

gboolean lobster_interface_load (const char *interface, GError **error);
gboolean lobster_interface_save (LobsterInterface *iface, GError **error);
void     lobster_interface_free (LobsterInterface *iface);
gboolean lobster_interface_renew (GError **error);

LobsterInterface *lobster_interface_get_from_device (const char *interface);
LobsterInterface *lobster_interface_get_selected (void);
void              lobster_interface_display_selected (void);

char *lobster_text_view_text (const char *name);

#define WIDGET(w) (lookup_widget (lobster.dialog, (w)))
#define BUFFER(w) (gtk_text_view_get_buffer (GTK_TEXT_VIEW (WIDGET (w))))
#define TEXT(w, v) G_STMT_START {                                       \
        GtkWidget *wid = WIDGET (w);                                    \
        if (GTK_IS_ENTRY (wid)) {                                       \
            gtk_entry_set_text (GTK_ENTRY (wid), (v) ? (v) : "");       \
        } else if (GTK_IS_TEXT_VIEW (wid)) {                            \
            gtk_text_buffer_set_text (BUFFER (w), (v) ? (v) : "", -1);  \
        }                                                               \
    } G_STMT_END;
#define EDITABLE(w, v) G_STMT_START {                                   \
        GtkWidget *wid = WIDGET (w);                                    \
        if (GTK_IS_EDITABLE (wid)) {                                    \
            gtk_editable_set_editable (GTK_EDITABLE (wid), (v));        \
        } else if (GTK_IS_TEXT_VIEW (wid)) {                            \
            gtk_text_view_set_editable (GTK_TEXT_VIEW (wid), (v));      \
        }                                                               \
    } G_STMT_END;
#define TOGGLED(w, v) (gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (WIDGET (w)), (v)))
#define ISTOGGLED(w) (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (WIDGET (w))))
#define ENABLED(w, v) (gtk_widget_set_sensitive (WIDGET (w), (v)))
#define STARTSWITH(s1,s2) (0 == strncmp (s1, s2, strlen (s2)))

#ifndef g_timeout_add_seconds
#define g_timeout_add_seconds(interval,function,data) g_timeout_add((interval)*1000,(function),(data))
#endif

G_END_DECLS

#endif /* LOBSTER_H */
