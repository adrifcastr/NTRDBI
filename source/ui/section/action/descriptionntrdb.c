#include <stdio.h>

#include <3ds.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "action.h"
#include "descriptionntrdb.h"
#include "../task/task.h"
#include "../../list.h"
#include "../../prompt.h"
#include "../../ui.h"
#include "../../error.h"
#include "../../../core/screen.h"

void description_ntrdb_open(linked_list* items, list_item* selected) {
	ntrdb_info* data = (ntrdb_info*) selected->data;

    if(data->meta.description == NULL) {
        error_display(NULL, NULL, "Failed to read NTRDB description data.");

        return;
    }

    list_display(data->meta.name, "B: Return", NULL, ntrdb_update, NULL);
}

static void ntrdb_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    if(hidKeysDown() & KEY_B) {
        ui_pop();
        return;
    }
	
	
}

static void ntrdb_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
    ui_draw_ntrdb_info(view, selected->data, x1, y1, x2, y2);
}