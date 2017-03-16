#include <stdio.h>
#include <malloc.h>

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
#include "../../../core/linkedlist.h"

typedef struct {
    populate_ntrdb_data populateData;

    bool populated;
} ntrdb_data;

typedef struct {
    linked_list* items;
    list_item* selected;
} ntrdb_action_data;

static ntrdb_info* info = NULL;

char desc[0x200];


struct list_item_s lists[32];

void description_ntrdb_open(linked_list* items, list_item* selected) {
	info = (ntrdb_info*) selected->data;
	ntrdb_action_data* data = (ntrdb_action_data*) calloc(1, sizeof(ntrdb_action_data));

    if(info == NULL) {
        error_display(NULL, NULL, "Failed to read plugin data.");
        return;
    }
	
	if(data == NULL) {
		error_display(NULL, NULL, "Failed to allocate NTR data.");
        return;
	}
	
	int i;
	for(i = 0; i < 32; i++) {
		strncpy(lists[i].name, " ", 512);
		lists[i].color = COLOR_TEXT;
		lists[i].data = NULL;
	}
	
	
	snprintf(desc, sizeof(desc),
             "%s",
             info->meta.description);
	
	data->items = items;
    data->selected = selected;
	
    list_display(info->meta.name, "B: Return", NULL, ntrdb_update, ntrdb_draw_top);
}

static void ntrdb_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
	ntrdb_action_data* actionData = (ntrdb_action_data*) data;

    if(hidKeysDown() & KEY_B) {
        ui_pop();
        list_destroy(view);
        free(data);
        return;
    }
	
	if(selected != NULL && selected->data != NULL && (selectedTouched || (hidKeysDown() & KEY_A))) {
        void(*action)(linked_list*, list_item*) = (void(*)(linked_list*, list_item*)) selected->data;

        ui_pop();
        list_destroy(view);

        action(actionData->items, actionData->selected);

        free(data);
        return;
    }
	
	if(linked_list_size(items) == 0) {
		char buf[0x200];
		strncpy(buf, desc, 0x200);
		
		int i = 0;
		char *p = strtok(buf, "\n");
		
		while (p != NULL){			
			strncpy(lists[i].name, p, 0x200);
			linked_list_add(items, &lists[i]);
			p = strtok(NULL, "\n");
			i++;
		}
    }
}

static void ntrdb_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
	ui_draw_ntrdb_info(view, info, x1, y1, x2, y2);
}