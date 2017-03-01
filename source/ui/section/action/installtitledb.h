#pragma once

#include "../../info.h"
void action_install_titledb(linked_list* items, list_item* selected);
void downloadPlugin(const char* text, const char* url, list_item* selected);
void downloadPlugin_confirmed(ui_view* view, void* data, bool response);
void startDownload(ui_view* view, void* data, float* progress, char* text);

