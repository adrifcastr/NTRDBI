#pragma once

#include "../../info.h"
void description_ntrdb_open(linked_list* items, list_item* selected);
static void ntrdb_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched);
static void ntrdb_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected);