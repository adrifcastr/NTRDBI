#include <sys/syslimits.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <3ds.h>

#include "task.h"
#include "../../list.h"
#include "../../error.h"
#include "../../../core/linkedlist.h"
#include "../../../core/screen.h"
#include "../../../core/util.h"

static Result task_populate_titles_add_ctr(populate_titles_data* data, FS_MediaType mediaType, u64 titleId) {
	return 0;
}

static Result task_populate_titles_add_twl(populate_titles_data* data, FS_MediaType mediaType, u64 titleId) {
    return 0;
}

static int task_populate_titles_compare_ids(const void* e1, const void* e2) {
    u64 id1 = *(u64*) e1;
    u64 id2 = *(u64*) e2;

    return id1 > id2 ? 1 : id1 < id2 ? -1 : 0;
}

static Result task_populate_titles_from(populate_titles_data* data, FS_MediaType mediaType, bool useDSiWare) {
    bool inserted;
    FS_CardType type;
    if(mediaType == MEDIATYPE_GAME_CARD && (R_FAILED(FSUSER_CardSlotIsInserted(&inserted)) || !inserted || R_FAILED(FSUSER_GetCardType(&type)))) {
        return 0;
    }

    Result res = 0;

    if(mediaType != MEDIATYPE_GAME_CARD || type == CARD_CTR) {
        u32 titleCount = 0;
        if(R_SUCCEEDED(res = AM_GetTitleCount(mediaType, &titleCount))) {
            u64* titleIds = (u64*) calloc(titleCount, sizeof(u64));
            if(titleIds != NULL) {
                if(R_SUCCEEDED(res = AM_GetTitleList(&titleCount, mediaType, titleCount, titleIds))) {
                    qsort(titleIds, titleCount, sizeof(u64), task_populate_titles_compare_ids);

                    for(u32 i = 0; i < titleCount && R_SUCCEEDED(res); i++) {
                        svcWaitSynchronization(task_get_pause_event(), U64_MAX);
                        if(task_is_quit_all() || svcWaitSynchronization(data->cancelEvent, 0) == 0) {
                            break;
                        }

                        if(data->filter == NULL || data->filter(data->userData, titleIds[i], mediaType)) {
                            bool dsiWare = ((titleIds[i] >> 32) & 0x8000) != 0;
                            if(dsiWare != useDSiWare) {
                                continue;
                            }

                            res = dsiWare ? task_populate_titles_add_twl(data, mediaType, titleIds[i]) : task_populate_titles_add_ctr(data, mediaType, titleIds[i]);
                        }
                    }
                }

                free(titleIds);
            } else {
                res = R_FBI_OUT_OF_MEMORY;
            }
        }
    } else {
        res = task_populate_titles_add_twl(data, mediaType, 0);
    }

    return res;
}

static void task_populate_titles_thread(void* arg) {
    populate_titles_data* data = (populate_titles_data*) arg;

    Result res = 0;

    if(R_SUCCEEDED(res = task_populate_titles_from(data, MEDIATYPE_GAME_CARD, false))) {
        if(R_SUCCEEDED(res = task_populate_titles_from(data, MEDIATYPE_SD, false))) {
            if(R_SUCCEEDED(res = task_populate_titles_from(data, MEDIATYPE_NAND, false))) {
                res = task_populate_titles_from(data, MEDIATYPE_NAND, true);
            }
        }
    }

    svcCloseHandle(data->cancelEvent);

    data->result = res;
    data->finished = true;
}

void task_free_title(list_item* item) {
    if(item == NULL) {
        return;
    }

    if(item->data != NULL) {
        title_info* titleInfo = (title_info*) item->data;
        if(titleInfo->hasMeta) {
            screen_unload_texture(titleInfo->meta.texture);
        }

        free(item->data);
    }

    free(item);
}

void task_clear_titles(linked_list* items) {
    if(items == NULL) {
        return;
    }

    linked_list_iter iter;
    linked_list_iterate(items, &iter);

    while(linked_list_iter_has_next(&iter)) {
        list_item* item = (list_item*) linked_list_iter_next(&iter);

        linked_list_iter_remove(&iter);
        task_free_title(item);
    }
}

Result task_populate_titles(populate_titles_data* data) {
    if(data == NULL || data->items == NULL) {
        return R_FBI_INVALID_ARGUMENT;
    }

    task_clear_titles(data->items);

    data->finished = false;
    data->result = 0;
    data->cancelEvent = 0;

    Result res = 0;
    if(R_SUCCEEDED(res = svcCreateEvent(&data->cancelEvent, RESET_STICKY))) {
        if(threadCreate(task_populate_titles_thread, data, 0x10000, 0x19, 1, true) == NULL) {
            res = R_FBI_THREAD_CREATE_FAILED;
        }
    }

    if(R_FAILED(res)) {
        data->finished = true;

        if(data->cancelEvent != 0) {
            svcCloseHandle(data->cancelEvent);
            data->cancelEvent = 0;
        }
    }

    return res;
}