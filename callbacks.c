#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "lobster.h"

void
on_dhcp_toggle_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gboolean enabled = GTK_WIDGET_IS_SENSITIVE (togglebutton) && gtk_toggle_button_get_active (togglebutton);
    ENABLED ("renew_button", enabled);

    enabled = GTK_WIDGET_IS_SENSITIVE (togglebutton) && !enabled;
    ENABLED ("address_entry", enabled);
    ENABLED ("subnet_entry", enabled);
    ENABLED ("router_entry", enabled);
    ENABLED ("dns_text", enabled);
    lobster_interface_dirty ();
}


void
on_connection_list_changed             (GtkComboBox     *combobox,
                                        gpointer         user_data)
{
    lobster_interface_display_selected ();
}

void
on_nm_toggle_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gboolean enabled = !gtk_toggle_button_get_active (togglebutton);
    /*ENABLED ("connection_list", enabled);*/
    ENABLED ("enable_toggle", enabled);
    on_enable_toggle_toggled (GTK_TOGGLE_BUTTON (WIDGET ("enable_toggle")), NULL);
    lobster_system_dirty ();
}


void
on_renew_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
    GError *error = NULL;
    if (!lobster_interface_renew (&error)) {
        lobster_show_error (_("<b>Could not renew DHCP configuration:</b>"), error);
        g_error_free (error);
    }
}


void
on_enable_toggle_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data)
{
    gboolean enabled = ISTOGGLED ("nm_toggle") || gtk_toggle_button_get_active (togglebutton);
    ENABLED ("dhcp_toggle", enabled);
    on_dhcp_toggle_toggled (GTK_TOGGLE_BUTTON (WIDGET ("dhcp_toggle")), NULL);
    /* above dirties interface */
}

void
on_revert_button_clicked               (GtkButton       *button,
                                        gpointer         user_data)
{
    GError *error = NULL;
    if (!lobster_system_load (&error)) {
        lobster_show_error (_("<b>Could not load network configuration; configuration may be incomplete:</b>"), error);
        g_error_free (error);
        /* should this exit? */
    }
    lobster_system_display ();
}

void
on_apply_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
    GError *error = NULL;
    if (!lobster_system_save (&error)) {
        lobster_show_error (_("<b>Could not save network configuration:</b>"), error);
        g_error_free (error);
    }
    ENABLED ("network_revert_button", FALSE);
    ENABLED ("network_apply_button", FALSE);    
}


void
on_close_button_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{
    GError *error = NULL;
    if (lobster_is_dirty ()) {
        GtkWidget *dialog = create_changes_dialog ();
        int res = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        switch (res) {
        case GTK_RESPONSE_APPLY:
            if (!lobster_is_valid ()) {
                return;
            }
            if (!lobster_system_save (&error)) {
                lobster_show_error (_("<b>Could not save network configuration:</b>"), error);
                g_error_free (error);
                return;
            }
            break;
        case GTK_RESPONSE_REJECT:
            break;
        case GTK_RESPONSE_CANCEL:
            return;
        default:
            g_assert_not_reached ();
            break;
        }
    }
    gtk_main_quit ();
}


void
on_address_entry_changed               (GtkEditable     *editable,
                                        gpointer         user_data)
{
    lobster_interface_dirty ();
}


void
on_subnet_entry_changed                (GtkEditable     *editable,
                                        gpointer         user_data)
{
    lobster_interface_dirty ();
}


void
on_router_entry_changed                (GtkEditable     *editable,
                                        gpointer         user_data)
{
    lobster_system_dirty ();
}

void
on_dns_text_changed                    (GtkTextBuffer   *buffer,
                                        gpointer         user_data)
{
    lobster_system_dirty ();
}
