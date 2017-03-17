#pragma once

typedef struct linked_list_s linked_list;
typedef struct list_item_s list_item;

void action_url_install(const char* confirmMessage, const char* urls, void* finishedData, void (*finished)(void* data));

void action_install_ntrdb(linked_list* items, list_item* selected);
void description_ntrdb_open(linked_list* items, list_item* selected);
void moreinfo_ntrdb_open(linked_list* items, list_item* selected);
void moreinfo_qr_ntrdb_open(linked_list* items, list_item* selected);
