#include <stdio.h>
#include <malloc.h>

#include <3ds.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "action.h"
#include "description.h"
#include "../task/task.h"
#include "../../list.h"
#include "../../prompt.h"
#include "../../ui.h"
#include "../../error.h"
#include "../../../core/screen.h"
#include "../../../core/linkedlist.h"
#include "../../../core/util.h"
#include "../../../stb_image/stb_image.h"


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

static Result task_populate_ntrdb_download(u32* downloadSize, void* buffer, u32 maxSize, const char* url) {
    Result res = 0;

    httpcContext context;
    if(R_SUCCEEDED(res = util_http_open(&context, NULL, url, true))) {
        res = util_http_read(&context, downloadSize, buffer, maxSize);

        Result closeRes = util_http_close(&context);
        if(R_SUCCEEDED(res)) {
            res = closeRes;
        }
    }

    return res;
}

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
	
	u32 maxPngSize = 128 * 1024;
	u8* png = (u8*) calloc(1, maxPngSize);
	if(png != NULL) {
		char pngUrl[128];
		snprintf(pngUrl, sizeof(pngUrl), "%s", info->meta.pic);

		u32 pngSize = 0;
		if(R_SUCCEEDED(task_populate_ntrdb_download(&pngSize, png, maxPngSize, pngUrl))) {
			int width;
			int height;
			int depth;
			u8* image = stbi_load_from_memory(png, (int) pngSize, &width, &height, &depth, STBI_rgb_alpha);
			if(image != NULL && depth == STBI_rgb_alpha) {
				for(u32 x = 0; x < width; x++) {
					for(u32 y = 0; y < height; y++) {
						u32 pos = (y * width + x) * 4;

						u8 c1 = image[pos + 0];
						u8 c2 = image[pos + 1];
						u8 c3 = image[pos + 2];
						u8 c4 = image[pos + 3];

						image[pos + 0] = c4;
						image[pos + 1] = c3;
						image[pos + 2] = c2;
						image[pos + 3] = c1;
					}
				}

				info->meta.pictexture = screen_allocate_free_texture();
				screen_load_texture(info->meta.pictexture, image, (u32) (width * height * 4), (u32) width, (u32) height, GPU_RGBA8, false);

				free(image);
			}
		}

		free(png);
	}
	
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
	//ui_draw_ntrdb_info(view, info, x1, y1, x2, y2);
	u32 iconWidth;
	u32 iconHeight;
	screen_get_texture_size(&iconWidth, &iconHeight, info->meta.pictexture);

	screen_draw_texture(info->meta.pictexture, 0, 0, iconWidth, iconHeight);
}