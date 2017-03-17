#include <stdio.h>
#include <malloc.h>

#include <3ds.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "action.h"
#include "moreinfo_qr.h"
#include "moreinfontrdb.h"
#include "../task/task.h"
#include "../../list.h"
#include "../../prompt.h"
#include "../../ui.h"
#include "../../error.h"
#include "../../../core/screen.h"
#include "../../../core/linkedlist.h"

static const char nametxt[] = "Name: ";
static const char compatibilitytxt[] = "Compatible with: ";
static const char developertxt[] = "Created by: ";
static const char devsitetxt[] = "Developer website: ";
static const char addedtxt[] = "Added: ";

static char nameval[0x100] = "";
static char compatibilityval[0x100] = "";
static char developerval[0x100] = "";
static char devsiteval[0x100] = "";
static char addedval[0x100] = "";

static list_item namelist = {" ", COLOR_TEXT, NULL};
static list_item compatibilitylist = {" ", COLOR_TEXT, NULL};
static list_item developerlist = {" ", COLOR_TEXT, NULL};
static list_item devsitelist = {" ", COLOR_TEXT, moreinfo_qr_ntrdb_open};
static list_item addedlist = {" ", COLOR_TEXT, NULL};

typedef struct {
    populate_ntrdb_data populateData;

    bool populated;
} ntrdb_data;

typedef struct {
    linked_list* items;
    list_item* selected;
} ntrdb_action_data;

ntrdb_info* ntrdbInfo = NULL;

void moreinfo_ntrdb_open(linked_list* items, list_item* selected) {

	ntrdbInfo = (ntrdb_info*) selected->data;
	ntrdb_action_data* data = (ntrdb_action_data*) calloc(1, sizeof(ntrdb_action_data));

    if(ntrdbInfo == NULL) {
        error_display(NULL, NULL, "Failed to read plugin data.");
        return;
    }
	
	if(data == NULL) {
		error_display(NULL, NULL, "Failed to allocate NTR data.");
        return;
	}
	
	snprintf(nameval, sizeof(nameval),
			"%s%s", 
			nametxt,
			ntrdbInfo->meta.name);
			
	snprintf(compatibilityval, sizeof(compatibilityval), 
			"%s%s", 
			compatibilitytxt, ntrdbInfo->meta.compatible);
			
	snprintf(developerval, sizeof(developerval),
			"%s%s", 
			developertxt, ntrdbInfo->meta.developer);
			
	snprintf(devsiteval, sizeof(devsiteval),
			"%s%s", 
			devsitetxt, ntrdbInfo->meta.devsite);
			
	snprintf(addedval, sizeof(addedval), 
			"%s%s", 
			addedtxt, ntrdbInfo->meta.added);
			
			
	strncpy(namelist.name, nameval, sizeof(nameval));
	strncpy(compatibilitylist.name, compatibilityval, sizeof(compatibilityval));
	strncpy(developerlist.name, developerval, sizeof(developerval));
	strncpy(devsitelist.name, devsiteval, sizeof(devsiteval));
	strncpy(addedlist.name, addedval, sizeof(addedval));

	data->items = items;
    data->selected = selected;
	
    list_display(ntrdbInfo->meta.name, "B: Return", NULL, ntrdb_update, ntrdb_draw_top);
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
        linked_list_add(items, &namelist);
		linked_list_add(items, &compatibilitylist);
		linked_list_add(items, &developerlist);
		linked_list_add(items, &devsitelist);
		linked_list_add(items, &addedlist);
    }
}

static void ntrdb_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
	ui_draw_ntrdb_info(view, ntrdbInfo, x1, y1, x2, y2);
}