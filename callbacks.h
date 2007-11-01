#include <gtk/gtk.h>


void
on_dhcp_toggle_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_connection_list_changed             (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_nm_toggle_toggled                   (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_renew_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_enable_toggle_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_apply_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_close_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_address_entry_changed               (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_subnet_entry_changed                (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_router_entry_changed                (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_dns_text_changed                    (GtkTextBuffer   *buffer,
                                        gpointer         user_data);

void
on_revert_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_apply_button_clicked                (GtkButton       *button,
                                        gpointer         user_data);
