/*
 * pidgin-regex-text-replacement
 *
 * Copyright (C) 2025 Olaf Wintermann
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ui.h"

/*
 * GtkTreeView widget, that contains a list of pattern/replacements
 */
static GtkWidget *treeview;

/*
 * treeview list store
 * col0: pattern string
 * col1: replacement string
 */
static GtkListStore *liststore;


static GtkWidget* create_treeview(void);
static void update_liststore(TextReplacementRule *rules, size_t numrules);

static void pattern_edited(GtkCellRendererText* self, gchar* path, gchar* new_text, gpointer user_data);
static void preplacement_edited(GtkCellRendererText* self, gchar* path, gchar* new_text, gpointer user_data);

static void add_button_clicked(GtkWidget *widget, void *userdata);
static void remove_button_clicked(GtkWidget *widget, void *userdata);

static int rules_modified = 0;

static void cleanup_ui(GtkWidget *object, void *userdata) {
    printf("cleanup_ui\n");
    
    treeview = NULL;
    liststore = NULL;
    
    if(rules_modified) {
        save_rules();
    }
    rules_modified = 0;
}

GtkWidget *get_config_frame(PurplePlugin *plugin) {
    // create ui
    GtkWidget *grid = gtk_table_new(1, 1, FALSE);
    gtk_table_set_col_spacings(GTK_TABLE(grid), 8);
    gtk_table_set_row_spacings(GTK_TABLE(grid), 8);
    
    GtkWidget *view = create_treeview();
    GtkAttachOptions xoptions = GTK_FILL | GTK_EXPAND;
    GtkAttachOptions yoptions = GTK_FILL | GTK_EXPAND;
    GtkWidget *scroll_area = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(
            GTK_SCROLLED_WINDOW(scroll_area),
            GTK_POLICY_AUTOMATIC,
            GTK_POLICY_AUTOMATIC); 
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll_area), view);
    
    gtk_table_attach(GTK_TABLE(grid), scroll_area, 0, 1, 0, 1, xoptions, yoptions, 0, 0);
    
    GtkWidget *hbox = gtk_hbox_new(TRUE, 8);
    GtkWidget *add = gtk_button_new_with_label("Add");
    GtkWidget *remove = gtk_button_new_with_label("Remove (TODO)");
    gtk_container_add(GTK_CONTAINER(hbox), add);
    gtk_container_add(GTK_CONTAINER(hbox), remove);
    
    g_signal_connect(
                add,
                "clicked",
                G_CALLBACK(add_button_clicked),
                NULL);
    g_signal_connect(
                remove,
                "clicked",
                G_CALLBACK(remove_button_clicked),
                NULL);
    
    gtk_table_attach(GTK_TABLE(grid), hbox, 0, 1, 1, 2, 0, 0, 0, 0);
    
    g_signal_connect(
                grid,
                "destroy",
                G_CALLBACK(cleanup_ui),
                NULL);
    
    // load data
    size_t nrules;
    TextReplacementRule *rules = get_rules(&nrules);
    update_liststore(rules, nrules);
    
    return grid;
}

static GtkWidget* create_treeview(void) {
    GtkWidget *view = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), TRUE);
    GtkCellRenderer *renderer0 = gtk_cell_renderer_text_new();
    GtkCellRenderer *renderer1 = gtk_cell_renderer_text_new();
    g_object_set(renderer0, "editable", TRUE, NULL);
    g_object_set(renderer1, "editable", TRUE, NULL);
    g_signal_connect(renderer0, "edited", G_CALLBACK(pattern_edited), NULL);
    g_signal_connect(renderer1, "edited", G_CALLBACK(preplacement_edited), NULL);
    GtkTreeViewColumn *column0 = gtk_tree_view_column_new_with_attributes(
                "Pattern",
                renderer0,
                "text",
                0,
                NULL);
    GtkTreeViewColumn *column1 = gtk_tree_view_column_new_with_attributes(
                "Replacement",
                renderer1,
                "text",
                1,
                NULL);
    gtk_tree_view_column_set_expand(column0, TRUE);
    gtk_tree_view_column_set_expand(column1, TRUE);
    gtk_tree_view_column_set_resizable(column0, TRUE);
    gtk_tree_view_column_set_resizable(column1, TRUE);
    
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column0);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column1);
    
    treeview = view;
    return view;
}


static void update_liststore(TextReplacementRule *rules, size_t numrules) {
    GType types[2] = { G_TYPE_STRING, G_TYPE_STRING };
    liststore = gtk_list_store_newv(2, types);
    
    for(int i=0;i<numrules;i++) {
        printf("insert rule: %s    %s\n", rules[i].pattern, rules[i].replacement);
        GtkTreeIter iter;
        gtk_list_store_insert (liststore, &iter, i);
        GValue value = G_VALUE_INIT;
        g_value_init(&value, G_TYPE_STRING);
        g_value_set_string(&value, rules[i].pattern);
        gtk_list_store_set_value(liststore, &iter, 0, &value);
        
        GValue value2 = G_VALUE_INIT;
        g_value_init(&value2, G_TYPE_STRING);
        g_value_set_string(&value2, rules[i].replacement);
        gtk_list_store_set_value(liststore, &iter, 1, &value2);
    }
    
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(liststore));
    g_object_unref(G_OBJECT(liststore));
}

static void update_text(GtkListStore *store, gchar *path, int col, gchar *new_text) {
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store), &iter, path)) {
        gtk_list_store_set(store, &iter, col, new_text, -1);
    }
}

static void pattern_edited(GtkCellRendererText* self, gchar* path, gchar* new_text, gpointer user_data) {
    int index = atoi(path);
    update_text(liststore, path, 0, new_text);
    
    int compiled = rule_update_pattern(index, new_text);
    printf("update pattern[%d]: %d\n", index, compiled);
    rules_modified = 1;
}

static void preplacement_edited(GtkCellRendererText* self, gchar* path, gchar* new_text, gpointer user_data) {
    int index = atoi(path);
    update_text(liststore, path, 1, new_text);
    rule_update_replacement(index, new_text);
    printf("update replacement: %s\n", new_text);
    rules_modified = 1;
}

static void add_button_clicked(GtkWidget *widget, void *userdata) {
    add_empty_rule();
    size_t nrules;
    TextReplacementRule *rules = get_rules(&nrules);
    update_liststore(rules, nrules);
}

static void remove_button_clicked(GtkWidget *widget, void *userdata) {
    
}
