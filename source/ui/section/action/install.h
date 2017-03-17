#pragma once

#include "../../info.h"
void action_install_ntrdb(linked_list* items, list_item* selected);
void internal_downloadPlugin_start(const char* text, list_item* selected);
void internal_downloadPlugin_confirmed(ui_view* view, void* data, bool response);
void internal_downloadPlugin_download(ui_view* view, void* data, float* progress, char* text);

