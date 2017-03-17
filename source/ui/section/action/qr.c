#include <stdio.h>
#include <malloc.h>

#include <3ds.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "action.h"
#include "qr.h"
#include "../task/task.h"
#include "../../list.h"
#include "../../prompt.h"
#include "../../ui.h"
#include "../../error.h"
#include "../../../core/screen.h"
#include "../../../core/linkedlist.h"

static list_item urllist = {" ", COLOR_TEXT, NULL};

typedef struct {
    populate_ntrdb_data populateData;

    bool populated;
} ntrdb_data;

typedef struct {
    linked_list* items;
    list_item* selected;
} ntrdb_action_data;


void qr_ntrdb_open(linked_list* items, list_item* selected) {	
	ntrdb_info* ntrdbInfo = (ntrdb_info*) selected->data;
	ntrdb_action_data* data = (ntrdb_action_data*) calloc(1, sizeof(ntrdb_action_data));

    if(ntrdbInfo == NULL) {
        error_display(NULL, NULL, "Failed to read plugin data.");
        return;
    }
	
	if(data == NULL) {
		error_display(NULL, NULL, "Failed to allocate NTR data.");
        return;
	}
	
	char devsite[0x100] = {0}; 
	snprintf(devsite, sizeof(devsite),
			"%s",
			ntrdbInfo->meta.devsite);
			
	strncpy(urllist.name, devsite, sizeof(devsite));

	data->items = items;
    data->selected = selected;
	
    list_display("Developer web site", "B: Return", NULL, ntrdb_update, ntrdb_draw_top);
}

static void ntrdb_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    if(hidKeysDown() & KEY_B) {
        ui_pop();
        list_destroy(view);
        free(data);
        return;
    }
		
	if(linked_list_size(items) == 0) {
		linked_list_add(items, &urllist);
    }
}

static void ntrdb_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {

}