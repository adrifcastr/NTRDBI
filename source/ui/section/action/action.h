#pragma once

typedef struct linked_list_s linked_list;
typedef struct list_item_s list_item;


void action_delete_file(linked_list* items, list_item* selected);
void action_delete_dir(linked_list* items, list_item* selected);
void action_delete_dir_contents(linked_list* items, list_item* selected);
void action_delete_dir_cias(linked_list* items, list_item* selected);
void action_delete_dir_tickets(linked_list* items, list_item* selected);
void action_new_folder(linked_list* items, list_item* selected);
void action_paste_contents(linked_list* items, list_item* selected);
void action_rename(linked_list* items, list_item* selected);


void action_delete_title(linked_list* items, list_item* selected);
void action_delete_title_ticket(linked_list* items, list_item* selected);

void action_url_install(const char* confirmMessage, const char* urls, void* finishedData, void (*finished)(void* data));

void action_install_titledb(linked_list* items, list_item* selected);
