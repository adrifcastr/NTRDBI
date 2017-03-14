#include <malloc.h>
#include <stdio.h>

#include <3ds.h>

#include "section.h"
#include "action/action.h"
#include "task/task.h"
#include "../error.h"
#include "../list.h"
#include "../ui.h"
#include "../../core/linkedlist.h"
#include "../../core/screen.h"

static list_item download = {"Download", COLOR_TEXT, action_install_ntrdb};
static list_item description = {"Description", COLOR_TEXT, description_ntrdb_open};
static list_item info = {"More info", COLOR_TEXT, NULL}; //TODO

typedef struct {
    populate_ntrdb_data populateData;

    bool populated;
} ntrdb_data;

typedef struct {
    linked_list* items;
    list_item* selected;
} ntrdb_action_data;

static void ntrdb_action_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
    ui_draw_ntrdb_info(view, ((ntrdb_action_data*) data)->selected->data, x1, y1, x2, y2);
}

static void ntrdb_action_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
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
        linked_list_add(items, &download);
		linked_list_add(items, &description);
		linked_list_add(items, &info);
    }
}

static void ntrdb_action_open(linked_list* items, list_item* selected) {
    ntrdb_action_data* data = (ntrdb_action_data*) calloc(1, sizeof(ntrdb_action_data));
    if(data == NULL) {
        error_display(NULL, NULL, "Failed to allocate NTRDB action data.");

        return;
    }

    data->items = items;
    data->selected = selected;

    list_display("NTRDB Action", "A: Select, B: Return", data, ntrdb_action_update, ntrdb_action_draw_top);
}

static void ntrdb_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
    if(selected != NULL && selected->data != NULL) {
        ui_draw_ntrdb_info(view, selected->data, x1, y1, x2, y2);
    }
}

static void ntrdb_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    ntrdb_data* listData = (ntrdb_data*) data;

    if(hidKeysDown() & KEY_B) {
        if(!listData->populateData.finished) {
            svcSignalEvent(listData->populateData.cancelEvent);
            while(!listData->populateData.finished) {
                svcSleepThread(1000000);
            }
        }

        ui_pop();

        task_clear_ntrdb(items);
        list_destroy(view);

        free(listData);
        return;
    }

    if(!listData->populated || (hidKeysDown() & KEY_X)) {
        if(!listData->populateData.finished) {
            svcSignalEvent(listData->populateData.cancelEvent);
            while(!listData->populateData.finished) {
                svcSleepThread(1000000);
            }
        }

        listData->populateData.items = items;
        Result res = task_populate_ntrdb(&listData->populateData);
        if(R_FAILED(res)) {
            error_display_res(NULL, NULL, res, "Failed to initiate NTRDB list population.");
        }

        listData->populated = true;
    }

    if(listData->populateData.finished && R_FAILED(listData->populateData.result)) {
        error_display_res(NULL, NULL, listData->populateData.result, "Failed to populate NTRDB list.");

        listData->populateData.result = 0;
    }

    if(selected != NULL && selected->data != NULL && (selectedTouched || (hidKeysDown() & KEY_A))) {
        ntrdb_action_open(items, selected);
        return;
    }
}

void ntrdb_open() {
    ntrdb_data* data = (ntrdb_data*) calloc(1, sizeof(ntrdb_data));
    if(data == NULL) {
        error_display(NULL, NULL, "Failed to allocate NTRDB data.");

        return;
    }

    data->populateData.finished = true;

    list_display("NTRDB", "A: Select, B: Return, X: Refresh", data, ntrdb_update, ntrdb_draw_top);
}