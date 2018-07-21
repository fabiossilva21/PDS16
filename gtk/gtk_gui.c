#include <gtk/gtk.h>

int main(int argc, char *argv[]){
        GtkBuilder *builder;
        GtkWidget *window;

        gtk_init(&argc, &argv);

        builder = gtk_builder_new();
        gtk_builder_add_from_file (builder, "window_main.glade", NULL);

        window = GTK_WIDGET(gtk_builder_get_object(builder, "window_main"));
        gtk_builder_connect_signals(builder, NULL);

        g_object_unref(builder);

        gtk_widget_show(window);
        gtk_main();

        return 0;
}

// called when window is closed
void on_window_main_destroy(){
        gtk_main_quit();
}

void open_file_dialog_window(GtkWidget *button, gpointer window){
        GtkWidget *dialog;
        dialog = gtk_file_chooser_dialog_new("Open an .ASM file", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Ok", GTK_RESPONSE_OK, "_Cancel", GTK_RESPONSE_CANCEL, NULL);
        gtk_widget_show_all(dialog);
        gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
        if (resp == GTK_RESPONSE_OK){
                printf("%s\n", gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
        }
        gtk_widget_destroy(dialog);
}
