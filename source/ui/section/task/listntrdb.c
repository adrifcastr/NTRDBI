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
#include "../../../json/json.h"
#include "../../../stb_image/stb_image.h"
#include "ntrdb.h"

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

static int task_populate_ntrdb_compare(void* userData, const void* p1, const void* p2) {
    list_item* info1 = (list_item*) p1;
    list_item* info2 = (list_item*) p2;

    return strncasecmp(info1->name, info2->name, LIST_ITEM_NAME_MAX);
}

static void task_populate_ntrdb_thread(void* arg) {
    populate_ntrdb_data* data = (populate_ntrdb_data*) arg;

    Result res = 0;

    linked_list tempItems;
    linked_list_init(&tempItems);

    u32 maxTextSize = 128 * 1024;
    char* text = (char*) calloc(sizeof(char), maxTextSize);
    if(text != NULL) {
        u32 textSize = 0;
        if(R_SUCCEEDED(res = task_populate_ntrdb_download(&textSize, text, maxTextSize, NTR_API_URL))) {
            json_value* json = json_parse(text, textSize);
            if(json != NULL) {
                if(json->type == json_array) {
                    for(u32 i = 0; i < json->u.array.length && R_SUCCEEDED(res); i++) {
                        svcWaitSynchronization(task_get_pause_event(), U64_MAX);
                        if(task_is_quit_all() || svcWaitSynchronization(data->cancelEvent, 0) == 0) {
                            break;
                        }

                        json_value* val = json->u.array.values[i];
                        if(val->type == json_object) {
                            list_item* item = (list_item*) calloc(1, sizeof(list_item));
                            if(item != NULL) {
                                ntrdb_info* ntrdbInfo = (ntrdb_info*) calloc(1, sizeof(ntrdb_info));
                                if(ntrdbInfo != NULL) {
                                    for(u32 j = 0; j < val->u.object.length; j++) {
                                        char* name = val->u.object.values[j].name;
                                        u32 nameLen = val->u.object.values[j].name_length;
                                        json_value* subVal = val->u.object.values[j].value;
                                        if(subVal->type == json_string) {
                                            if(strncmp(name, "TitleID", nameLen) == 0) {
                                                ntrdbInfo->titleId = strtoull(subVal->u.string.ptr, NULL, 16);
                                            } else if(strncmp(name, "name", nameLen) == 0) {
                                                strncpy(ntrdbInfo->meta.name, subVal->u.string.ptr, sizeof(ntrdbInfo->meta.name));
                                            } else if(strncmp(name, "compatible", nameLen) == 0) {
												char* compatibility = NULL; 
												if(strcmp(subVal->u.string.ptr, "o3ds") == 0) {
														compatibility = "Old 3DS / 2DS";
												} else {
													if (strcmp(subVal->u.string.ptr, "n3ds") == 0) {
														compatibility = "New 3DS";
													} else {
														compatibility = "Universal";
													}
												}
                                                strncpy(ntrdbInfo->meta.compatible, compatibility, sizeof(ntrdbInfo->meta.compatible));
                                            } else if(strncmp(name, "desc", nameLen) == 0) {
												char* desc = subVal->u.string.ptr;
												int i;
												for(i = 0; desc[i] != '\0'; i++) {
													if((desc[i] == '\r') && (desc[i + 1] == '\n')) {
														desc[i] = '\n';
													}
												}
												for(i = 0; desc[i] != '\0'; i++) {
													if((desc[i] == '\n') && (desc[i + 1] == '\n')) {
														desc[i] = ' ';
													}
												}
												strncpy(ntrdbInfo->meta.description, desc, sizeof(ntrdbInfo->meta.description));
											} else if(strncmp(name, "developer", nameLen) == 0) {
                                                strncpy(ntrdbInfo->meta.developer, subVal->u.string.ptr, sizeof(ntrdbInfo->meta.developer));
                                            } else if(strncmp(name, "devsite", nameLen) == 0) {
                                                strncpy(ntrdbInfo->meta.devsite, subVal->u.string.ptr, sizeof(ntrdbInfo->meta.devsite));
                                            } else if(strncmp(name, "version", nameLen) == 0) {
                                                strncpy(ntrdbInfo->meta.version, subVal->u.string.ptr, sizeof(ntrdbInfo->meta.version));
											} else if(strncmp(name, "added", nameLen) == 0) {
                                                strncpy(ntrdbInfo->meta.added, subVal->u.string.ptr, sizeof(ntrdbInfo->meta.added));
											} else if(strncmp(name, "plg", nameLen) == 0) {
                                                strncpy(ntrdbInfo->downloadURL, subVal->u.string.ptr, sizeof(ntrdbInfo->downloadURL));
											} else if(strncmp(name, "pic", nameLen) == 0) {
                                                strncpy(ntrdbInfo->meta.pic, subVal->u.string.ptr, sizeof(ntrdbInfo->meta.pic));
											}
                                        }
                                    }

                                    if(strlen(ntrdbInfo->meta.name) > 0) {
                                        strncpy(item->name, ntrdbInfo->meta.name, LIST_ITEM_NAME_MAX);
                                    } else {
                                        snprintf(item->name, LIST_ITEM_NAME_MAX, "%016llX", ntrdbInfo->titleId);
                                    }

                                    item->color = COLOR_NOT_INSTALLED; //TODO

                                    item->data = ntrdbInfo;

                                    linked_list_add(&tempItems, item);
                                } else {
                                    free(item);

                                    res = R_FBI_OUT_OF_MEMORY;
                                }
                            } else {
                                res = R_FBI_OUT_OF_MEMORY;
                            }
                        }
                    }
                } else {
                    res = R_FBI_BAD_DATA;
                }
            } else {
                res = R_FBI_PARSE_FAILED;
            }
        }

        free(text);
    } else {
        res = R_FBI_OUT_OF_MEMORY;
    }

    if(R_SUCCEEDED(res)) {
        linked_list_sort(&tempItems, NULL, task_populate_ntrdb_compare);

        linked_list_iter tempIter;
        linked_list_iterate(&tempItems, &tempIter);

        while(linked_list_iter_has_next(&tempIter)) {
            linked_list_add(data->items, linked_list_iter_next(&tempIter));
        }

        linked_list_iter iter;
        linked_list_iterate(data->items, &iter);

        while(linked_list_iter_has_next(&iter)) {
            svcWaitSynchronization(task_get_pause_event(), U64_MAX);
            if(task_is_quit_all() || svcWaitSynchronization(data->cancelEvent, 0) == 0) {
                break;
            }

            list_item* item = (list_item*) linked_list_iter_next(&iter);
            ntrdb_info* ntrdbInfo = (ntrdb_info*) item->data;

            u32 maxPngSize = 128 * 1024;
            u8* png = (u8*) calloc(1, maxPngSize);
            if(png != NULL) {
                char pngUrl[128];
                snprintf(pngUrl, sizeof(pngUrl), "https://raw.githubusercontent.com/adrifcastr/NTRDB-Plugin-Host/master/images/%s.png", ntrdbInfo->meta.name);

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

                        ntrdbInfo->meta.texture = screen_allocate_free_texture();
                        screen_load_texture(ntrdbInfo->meta.texture, image, (u32) (width * height * 4), (u32) width, (u32) height, GPU_RGBA8, false);

                        free(image);
                    }
                }

                free(png);
            }
        }
    }

    linked_list_destroy(&tempItems);

    svcCloseHandle(data->cancelEvent);

    data->result = res;
    data->finished = true;
}

void task_free_ntrdb(list_item* item) {
    if(item == NULL) {
        return;
    }

    if(item->data != NULL) {
        ntrdb_info* ntrdbInfo = (ntrdb_info*) item->data;
        if(ntrdbInfo->meta.texture != 0) {
            screen_unload_texture(ntrdbInfo->meta.texture);
            ntrdbInfo->meta.texture = 0;
        }

        free(item->data);
    }

    free(item);
}

void task_clear_ntrdb(linked_list* items) {
    if(items == NULL) {
        return;
    }

    linked_list_iter iter;
    linked_list_iterate(items, &iter);

    while(linked_list_iter_has_next(&iter)) {
        list_item* item = (list_item*) linked_list_iter_next(&iter);

        linked_list_iter_remove(&iter);
        task_free_ntrdb(item);
    }
}

Result task_populate_ntrdb(populate_ntrdb_data* data) {
    if(data == NULL || data->items == NULL) {
        return R_FBI_INVALID_ARGUMENT;
    }

    task_clear_ntrdb(data->items);

    data->finished = false;
    data->result = 0;
    data->cancelEvent = 0;

    Result res = 0;
    if(R_SUCCEEDED(res = svcCreateEvent(&data->cancelEvent, RESET_STICKY))) {
        if(threadCreate(task_populate_ntrdb_thread, data, 0x10000, 0x19, 1, true) == NULL) {
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