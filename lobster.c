#include "config.h"

#include "lobster.h"

#include "lobsterio.h"

#include "support.h"
#include "interface.h"

#include <glib/gstring.h>

#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <wait.h>

#define NET_DEVICES "/proc/net/dev"
#define NETWORK_CONFIG "/etc/sysconfig/network/config"
#define NETWORK_IFCFG "/etc/sysconfig/network/ifcfg"
#define NETWORK_ROUTES "/etc/sysconfig/network/routes"
#define RESOLV_CONF "/etc/resolv.conf"

LobsterSystem lobster;

void
lobster_show_error (const char *doing, GError *error)
{
    GtkWidget *dialog;

    dialog = create_error_dialog ();
    gtk_label_set_markup (GTK_LABEL (lookup_widget (dialog, "error_label")), doing);
    gtk_text_buffer_set_text (gtk_text_view_get_buffer (GTK_TEXT_VIEW (lookup_widget (dialog, "error_text"))),
                              error && error->message ? error->message : _("No error message given."), -1);

    gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_destroy (dialog);
}

void
lobster_ignore_edits (void)
{
    ++lobster.ignore_edits;
}

void
lobster_accept_edits (void)
{
    --lobster.ignore_edits;
}

static gboolean
yesorno (const char *str)
{
    str = strchr (str, '=');
    if (!str) {
        return FALSE;
    }
    ++str;
    if (*str == '"') {
        ++str;
    }
    return STARTSWITH (str, "yes");
}

static char *
view_text (const char *name)
{
    GtkTextBuffer *buf = BUFFER (name);
    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter (buf, &start);
    gtk_text_buffer_get_end_iter (buf, &end);
    return gtk_text_buffer_get_text (buf, &start, &end, FALSE);
}

static gboolean
read_net_devices (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    char *colon;
    while (*line && *line == ' ') {
        ++line;
    }
    if (!STARTSWITH (line, "eth")) {
        return TRUE;
    }
    colon = strchr (line, ':');
    if (!colon) {
        return TRUE;
    }
    *colon = '\0';
    fprintf (stderr, "%s:%d: %s\n", file, line_no, line);
    return lobster_interface_load (line, error);
}

static gboolean
read_dns_servers (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    if (!STARTSWITH (line, "nameserver ")) {
        return TRUE;
    }
    fprintf (stderr, "%s:%d: %s\n", file, line_no, line);
    line = strchr (line, ' ');
    if (line) {
        g_string_append ((GString *)data, line + 1);
        g_string_append ((GString *)data, "\n");
    }
    return TRUE;
}

static char *
write_dns_servers (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    if (line == NULL) {
        GString *buf = g_string_new (NULL);
        char **servers = g_strsplit_set (lobster.dns_servers, " \n\t\r,", -1);
        int i;
        for (i = 0; servers[i]; i++) {
            if (*servers[i]) {
                if (i) {
                    g_string_append (buf, "\n");
                }
                g_string_append (buf, "nameserver ");
                g_string_append (buf, servers[i]);
            }
        }
        fprintf (stderr, "%s: appending %s\n", file, buf->str);
        return g_string_free (buf, FALSE);
    } else if (STARTSWITH (line, "nameserver ")) {
        fprintf (stderr, "%s:%d: skipping %s\n", file, line_no, line);
        return g_strdup ("");
    }
    fprintf (stderr, "%s:%d: keeping %s\n", file, line_no, line);
    return line;
}

static gboolean
check_for_nm (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    if (!STARTSWITH (line, "NETWORKMANAGER=")) {
        return TRUE;
    }
    lobster.use_nm = yesorno (line);
    fprintf (stderr, "%s:%d: %s -> %d\n", file, line_no, line, lobster.use_nm);
    return TRUE;
}

static char *
write_nm (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    gboolean *written = data;
    if (!line) {
        if (*written) {
            return g_strdup ("");
        }
    } else if (!STARTSWITH (line, "NETWORKMANAGER=")) {
        return line;
    }
    *written = TRUE;
    return g_strdup_printf ("NETWORKMANAGER=\"%s\"", lobster.use_nm ? "yes" : "no");
}

static gboolean
read_routes (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    char *eol;
    if (STARTSWITH (line, "default ")) {
        line = strchr (line, ' ') + 1;
        eol = strchr (line, ' ');
        if (eol) {
            *eol = '\0';
        }
    } else if (strchr (line, ' ')) {
        return TRUE;
    }
    g_free (lobster.router);
    lobster.router = g_strdup (line);
    return TRUE;
}

static char *
write_routes (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    gboolean *written = data;
    if (!line) {
        if (*written) {
            return g_strdup ("");
        }
    } else if (!STARTSWITH (line, "default ") || strchr (line, ' ')) {
        return line;
    }
    *written = TRUE;
    return g_strdup_printf ("default %s", lobster.router);
}

gboolean
lobster_system_load (GError **error)
{
    GString *servers;
    GList *li;

    /* lobster.interfaces */
    g_list_foreach (lobster.interfaces, (GFunc)lobster_interface_free, NULL);
    g_list_free (lobster.interfaces);
    lobster.interfaces = NULL;
    if (!lobster_io_read_file (NET_DEVICES, read_net_devices, NULL, error)) {
        return FALSE;
    }

    /* lobster.dns_servers */
    servers = g_string_new (NULL);
    if (!lobster_io_read_file (RESOLV_CONF, read_dns_servers, servers, error)) {
        return FALSE;
    }
    g_free (lobster.dns_servers);
    lobster.dns_servers = g_string_free (servers, FALSE);

    fprintf (stderr, "have nameservers: %s\n", lobster.dns_servers);

    /* lobster.router */
    if (!lobster_io_read_file (NETWORK_ROUTES, read_routes, NULL, error)) {
        return FALSE;
    }

    fprintf (stderr, "router: %s\n", lobster.router);

    /* lobster.use_nm */
    if (!lobster_io_read_file (NETWORK_CONFIG, check_for_nm, NULL, error)) {
        return FALSE;
    }

    lobster.dirty = FALSE;

    return TRUE;
}

static void
apply_finished (GPid pid, gint status, gpointer data)
{
    *(int *)data = status;
    gtk_main_quit ();
    fprintf (stderr, "child finished: %d\n", WEXITSTATUS (status));
}

static gboolean
show_dialog (gpointer data)
{
    GtkWidget *dialog = create_applying_dialog ();
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (lobster.dialog));
    gtk_widget_show_all (dialog);
    *(GtkWidget **)data = dialog;
    fprintf (stderr, "creating dialog...\n");
    return FALSE;
}

static gboolean
lobster_system_apply (GError **error)
{
    GtkWidget *dialog = NULL;
    guint dialog_id;
    GPid pid;
    int status = -1;

    char *argv[4] = { "/sbin/service", "network", "restart", NULL };

    ENABLED ("network_dialog", FALSE);

    if (!g_spawn_async ("/", argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, error)) {
        fprintf (stderr, "could not spawn network\n");
        ENABLED ("network_dialog", TRUE);
        return FALSE;
    }

    dialog_id = g_timeout_add_seconds (2, show_dialog, &dialog);
    g_child_watch_add (pid, apply_finished, &status);

    gtk_main ();

    if (dialog) {
        gtk_widget_destroy (dialog);
    } else {
        g_source_remove (dialog_id);
    }

    ENABLED ("network_dialog", TRUE);

    return WEXITSTATUS (status) == 0;
}

gboolean
lobster_system_save (GError **error)
{
    GList *li;
    gboolean written_nm = FALSE;
    gboolean written_routes = FALSE;

    /* lobster.interfaces */
    for (li = lobster.interfaces; li; li = li->next) {
        if (!lobster_interface_save (li->data, error)) {
            return FALSE;
        }
    }

    if (!lobster.dirty) {
        fprintf (stderr, "system not dirty\n");
        goto apply_settings;
    }

    /* lobster.dns_servers */
    g_free (lobster.dns_servers);
    lobster.dns_servers = view_text ("dns_text");
    if (!lobster_io_overwrite_file (RESOLV_CONF, write_dns_servers, NULL, error)) {
        return FALSE;
    }

    /* lobster.router */
    g_free (lobster.router);
    lobster.router = g_strdup (gtk_entry_get_text (GTK_ENTRY (WIDGET ("router_entry"))));
    if (!lobster_io_overwrite_file (NETWORK_ROUTES, write_routes, &written_routes, error)) {
        return FALSE;
    }

    /* lobster.use_nm */
    lobster.use_nm = ISTOGGLED ("nm_toggle");
    if (!lobster_io_overwrite_file (NETWORK_CONFIG, write_nm, &written_nm, error)) {
        return FALSE;
    }

    lobster.dirty = FALSE;

apply_settings:
    return lobster_system_apply (error);
}

static char *
device_text (LobsterInterface *iface)
{
    return g_strdup (iface->interface);
}

void
lobster_system_display (void)
{
    GList *li;
    GtkComboBox *combo;
    char *iface;

    lobster_ignore_edits ();

    TOGGLED ("nm_toggle", lobster.use_nm);

    combo = GTK_COMBO_BOX (WIDGET ("connection_list"));
    /* there doesn't seem to be a "clear" method, so poach the store
     * creation from _new_text() */
    gtk_combo_box_set_model (combo, GTK_TREE_MODEL (gtk_list_store_new (1, G_TYPE_STRING)));
    for (li = lobster.interfaces; li; li = li->next) {
        iface = device_text ((LobsterInterface *)li->data);
        gtk_combo_box_append_text (combo, iface);
        g_free (iface);
    }
    gtk_combo_box_set_active (combo, 0);

    ENABLED ("network_revert_button", FALSE);
    ENABLED ("network_apply_button", FALSE);

    lobster_accept_edits ();
}

void
lobster_system_dirty (void)
{
    if (!lobster.ignore_edits) {
        lobster.dirty = TRUE;
        ENABLED ("network_revert_button", TRUE);
        ENABLED ("network_apply_button", TRUE);
    } else {
        fprintf (stderr, "ignoring system edit\n");
    }
}

void
lobster_interface_dirty (void)
{
    if (!lobster.ignore_edits) {
        LobsterInterface *iface = lobster_interface_get_selected ();
        if (iface) {
            iface->dirty = TRUE;
            ENABLED ("network_revert_button", TRUE);
            ENABLED ("network_apply_button", TRUE);
        }
    } else {
        fprintf (stderr, "ignoring interface edit\n");
    }
}

gboolean
lobster_is_dirty (void)
{
    GList *li;
    if (lobster.dirty) {
        return TRUE;
    }
    for (li = lobster.interfaces; li; li = li->next) {
        if (((LobsterInterface *)li->data)->dirty) {
            return TRUE;
        }
    }
    return FALSE;
}

typedef enum {
    LINE_UNKNOWN,
    LINE_STARTMODE,
    LINE_BOOTPROTO,
    LINE_IPADDR,
    LINE_NETMASK
} LineType;

static char *
trim_quotes (char *line)
{
    char *ret = line;
    line = strchr (ret, '\'');
    if (line) {
        *line = '\0';
    }
    line = strchr (ret, '"');
    if (line) {
        *line = '\0';
    }
    return ret;
}

static gboolean
interface_read_func (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    LobsterInterface *iface = (LobsterInterface *)data;
    LineType type = LINE_UNKNOWN;

    if (STARTSWITH (line, "STARTMODE=")) {
        type = LINE_STARTMODE;
    } else if (STARTSWITH (line, "BOOTPROTO=")) {
        type = LINE_BOOTPROTO;
    } else if (STARTSWITH (line, "IPADDR=")) {
        type = LINE_IPADDR;
    } else if (STARTSWITH (line, "NETMASK=")) {
        type = LINE_NETMASK;
    } else {
        return TRUE;
    }

    fprintf (stderr, "%s:%d: %s => %d\n", file, line_no, line, type);
    line = strchr (line, '=');
    if (!line) {
        return;
    }

    line++;
    if (*line == '\'' || *line == '"') {
        line++;
    }

    switch (type) {
    case LINE_STARTMODE:
        iface->enabled = !STARTSWITH (line, "off");
        break;
    case LINE_BOOTPROTO:
        iface->dhcp = !STARTSWITH (line, "static");
        break;
    case LINE_IPADDR:
        g_free (iface->address);
        iface->address = g_strdup (trim_quotes (line));
        break;
    case LINE_NETMASK:
        g_free (iface->subnet);
        iface->subnet = g_strdup (trim_quotes (line));
        break;
    default:
        g_assert_not_reached ();
        break;
    }
    return TRUE;
}

gboolean
lobster_interface_load (const char *interface, GError **error)
{
    LobsterInterface *iface = g_new0 (LobsterInterface, 1);
    char *file = g_strdup_printf ("%s-%s", NETWORK_IFCFG, interface);
    
    iface->interface = g_strdup (interface);
    if (!lobster_io_read_file (file, interface_read_func, iface, error)) {
        g_free (file);
        lobster_interface_free (iface);
        return FALSE;
    }
    g_free (file);

    fprintf (stderr, "got interface: %s %d %d %s %s\n", 
             iface->interface,
             iface->enabled,
             iface->dhcp,
             iface->address,
             iface->subnet);

    lobster.interfaces = g_list_append (lobster.interfaces, iface);

    return TRUE;
}

typedef struct {
    LobsterInterface *iface;
    gboolean wrote_address;
    gboolean wrote_subnet;
    gboolean wrote_enabled;
    gboolean wrote_dhcp;
} InterfaceWriteData;

static char *
interface_write_func (const char *file, int line_no, char *line, gpointer data, GError **error)
{
    InterfaceWriteData *iwd = data;
    LobsterInterface *iface = iwd->iface;

    if (!line) {
        GString *buf = g_string_new (NULL);

        if (!iwd->wrote_address) {
            g_string_append_printf (buf, "IPADDR='%s'\n", iface->dhcp ? "" : iface->address);
        }
        if (!iwd->wrote_subnet) {
            g_string_append_printf (buf, "NETMASK='%s'\n", iface->dhcp ? "" : iface->subnet);
        }
        if (!iwd->wrote_enabled) {
            g_string_append_printf (buf, "STARTMODE='%s'\n", iface->enabled ? "auto" : "off");
        }
        if (!iwd->wrote_dhcp) {
            g_string_append_printf (buf, "BOOTPROTO='%s'\n", iface->dhcp ? "dhcp+autoip" : "static");
        }
        return g_string_free (buf, FALSE);
    } else if (STARTSWITH (line, "IPADDR=")) {
        iwd->wrote_address = TRUE;
        return g_strdup_printf ("IPADDR='%s'", iface->dhcp ? "" : iface->address);
    } else if (STARTSWITH (line, "NETMASK=")) {
        iwd->wrote_subnet = TRUE;
        return g_strdup_printf ("NETMASK='%s'", iface->dhcp ? "" : iface->subnet);
    } else if (STARTSWITH (line, "STARTMODE=")) {
        iwd->wrote_enabled = TRUE;
        return g_strdup_printf ("STARTMODE='%s'", iface->enabled ? "auto" : "off");
    } else if (STARTSWITH (line, "BOOTPROTO=")) {
        iwd->wrote_dhcp = TRUE;
        return g_strdup_printf ("BOOTPROTO='%s'", iface->dhcp ? "dhcp+autoip" : "static");
    }
    return line;
}

gboolean
lobster_interface_save (LobsterInterface *iface, GError **error)
{
    InterfaceWriteData iwd = { iface, FALSE, FALSE, FALSE, FALSE };

    if (!iface->dirty) {
        fprintf (stderr, "%s: not dirty\n", iface->interface);
        return TRUE;
    }

    char *file = g_strdup_printf ("%s-%s", NETWORK_IFCFG, iface->interface);

    g_free (iface->address);
    iface->address = g_strdup (gtk_entry_get_text (GTK_ENTRY (WIDGET ("address_entry"))));

    g_free (iface->subnet);
    iface->subnet = g_strdup (gtk_entry_get_text (GTK_ENTRY (WIDGET ("subnet_entry"))));

    iface->enabled = ISTOGGLED ("enable_toggle");
    iface->dhcp = ISTOGGLED ("dhcp_toggle");

    iface->dirty = FALSE;

    if (!lobster_io_overwrite_file (file, interface_write_func, &iwd, error)) {
        g_free (file);
        return FALSE;
    }

    g_free (file);

    return TRUE;
}

void
lobster_interface_free (LobsterInterface *iface)
{
    g_free (iface->interface);
    iface->interface = NULL;
    
    g_free (iface->address);
    iface->address = NULL;

    g_free (iface->subnet);
    iface->subnet = NULL;

    g_free (iface);
}

LobsterInterface *
lobster_interface_get_from_device (const char *interface)
{
    LobsterInterface *iface;
    GList *li;
    for (li = lobster.interfaces; li; li = li->next) {
        iface = li->data;
        if (!strcmp (iface->interface, interface)) {
            return iface;
        }
    }
    return NULL;
}

LobsterInterface *
lobster_interface_get_selected (void)
{
    int active = gtk_combo_box_get_active (GTK_COMBO_BOX (WIDGET ("connection_list")));
    return active != -1 ? g_list_nth_data (lobster.interfaces, active) : NULL;
}

gboolean
lobster_interface_renew (GError **error)
{
    return FALSE;
}

void
lobster_interface_display_selected (void)
{
    LobsterInterface *iface = lobster_interface_get_selected ();

    lobster_ignore_edits ();

    TOGGLED ("enable_toggle", iface ? iface->enabled : FALSE);
    TOGGLED ("dhcp_toggle", iface ? iface->dhcp : TRUE);
    TEXT ("address_entry", iface ? iface->address : "");
    TEXT ("subnet_entry", iface ? iface->subnet : "");
    TEXT ("router_entry", iface ? lobster.router : "");
    TEXT ("dns_text", iface ? lobster.dns_servers : "");

    lobster_accept_edits ();
}
