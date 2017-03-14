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

char desc[0x200];

void description_ntrdb_open(linked_list* items, list_item* selected) {
	ntrdb_info* data = (ntrdb_info*) selected->data;

    if(data->meta.description == NULL) {
        error_display(NULL, NULL, "Failed to read NTRDB description data.");

        return;
    }
	
	snprintf(desc, sizeof(desc),
             "%s",
             data->meta.description);
	
    list_display(data->meta.name, "B: Return", NULL, ntrdb_update, ntrdb_draw_top);
}

static void ntrdb_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    if(hidKeysDown() & KEY_B) {
        ui_pop();
        return;
    }
}

static void ntrdb_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
	screen_draw_string(desc, 8, 25, 0.5f, 0.5f, COLOR_TEXT, false);
}