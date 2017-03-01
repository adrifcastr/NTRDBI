#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "section.h"
#include "task/task.h"
#include "../error.h"
#include "../info.h"
#include "../prompt.h"
#include "../ui.h"
#include "../../core/screen.h"
#include "../../core/util.h"
#include "../../json/json.h"

#define URL_MAX 1024

typedef struct {
    char url[URL_MAX];

    u32 responseCode;
    data_op_data installInfo;
} update_data;

static Result update_is_src_directory(void* data, u32 index, bool* isDirectory) {
    *isDirectory = false;
    return 0;
}

static Result update_make_dst_directory(void* data, u32 index) {
    return 0;
}

static Result update_open_src(void* data, u32 index, u32* handle) {
    update_data* updateData = (update_data*) data;

    Result res = 0;

    httpcContext* context = (httpcContext*) calloc(1, sizeof(httpcContext));
    if(context != NULL) {
        if(R_SUCCEEDED(res = util_http_open(context, &updateData->responseCode, updateData->url, true))) {
            *handle = (u32) context;
        } else {
            free(context);
        }
    } else {
        res = R_FBI_OUT_OF_MEMORY;
    }

    return res;
}

static Result update_close_src(void* data, u32 index, bool succeeded, u32 handle) {
    return util_http_close((httpcContext*) handle);
}

static Result update_get_src_size(void* data, u32 handle, u64* size) {
    u32 downloadSize = 0;
    Result res = util_http_get_size((httpcContext*) handle, &downloadSize);

    *size = downloadSize;
    return res;
}

static Result update_read_src(void* data, u32 handle, u32* bytesRead, void* buffer, u64 offset, u32 size) {
    return util_http_read((httpcContext*) handle, bytesRead, buffer, size);
}

static Result update_open_dst(void* data, u32 index, void* initialReadBlock, u64 size, u32* handle) {
    if(util_get_3dsx_path() != NULL) {
        FS_Path* path = util_make_path_utf8(util_get_3dsx_path());
        if(path != NULL) {
            Result res = FSUSER_OpenFileDirectly(handle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), *path, FS_OPEN_WRITE | FS_OPEN_CREATE, 0);

            util_free_path_utf8(path);
            return res;
        } else {
            return R_FBI_OUT_OF_MEMORY;
        }
    } else {
        return AM_StartCiaInstall(MEDIATYPE_SD, handle);
    }

}

static Result update_close_dst(void* data, u32 index, bool succeeded, u32 handle) {
    if(util_get_3dsx_path() != NULL) {
        return FSFILE_Close(handle);
    } else {
        if(succeeded) {
            return AM_FinishCiaInstall(handle);
        } else {
            return AM_CancelCIAInstall(handle);
        }
    }
}

static Result update_write_dst(void* data, u32 handle, u32* bytesWritten, void* buffer, u64 offset, u32 size) {
    return FSFILE_Write(handle, bytesWritten, offset, buffer, size, 0);
}

static Result update_suspend_copy(void* data, u32 index, u32* srcHandle, u32* dstHandle) {
    return 0;
}

static Result update_restore_copy(void* data, u32 index, u32* srcHandle, u32* dstHandle) {
    return 0;
}

static Result update_suspend(void* data, u32 index) {
    return 0;
}

static Result update_restore(void* data, u32 index) {
    return 0;
}

static bool update_error(void* data, u32 index, Result res) {
    update_data* updateData = (update_data*) data;

    if(res == R_FBI_CANCELLED) {
        prompt_display("Failure", "Install cancelled.", COLOR_TEXT, false, NULL, NULL, NULL);
    } else if(res == R_FBI_HTTP_RESPONSE_CODE) {
        error_display(NULL, NULL, "Failed to update NTRDBI.\nHTTP server returned response code %d", updateData->responseCode);
    } else {
        error_display_res(NULL, NULL, res, "Failed to update NTI.");
    }

    return false;
}

static void update_install_update(ui_view* view, void* data, float* progress, char* text) {
    update_data* updateData = (update_data*) data;

    if(updateData->installInfo.finished) {
        ui_pop();
        info_destroy(view);

        if(R_SUCCEEDED(updateData->installInfo.result)) {
            prompt_display("Success", "Update complete.", COLOR_TEXT, false, NULL, NULL, NULL);
        }

        free(updateData);

        return;
    }

    if(hidKeysDown() & KEY_B) {
        svcSignalEvent(updateData->installInfo.cancelEvent);
    }

    *progress = updateData->installInfo.currTotal != 0 ? (float) ((double) updateData->installInfo.currProcessed / (double) updateData->installInfo.currTotal) : 0;
    snprintf(text, PROGRESS_TEXT_MAX, "%.2f %s / %.2f %s\n%.2f %s/s", util_get_display_size(updateData->installInfo.currProcessed), util_get_display_size_units(updateData->installInfo.currProcessed), util_get_display_size(updateData->installInfo.currTotal), util_get_display_size_units(updateData->installInfo.currTotal), util_get_display_size(updateData->installInfo.copyBytesPerSecond), util_get_display_size_units(updateData->installInfo.copyBytesPerSecond));
}

static void update_check_update(ui_view* view, void* data, float* progress, char* text) {
    update_data* updateData = (update_data*) data;

    bool hasUpdate = false;

    Result res = 0;
    u32 responseCode = 0;

    httpcContext context;
    if(R_SUCCEEDED(res = util_http_open(&context, &responseCode, "https://raw.githubusercontent.com/adrifcastr/NTRDB-Plugin-Host/NTRDBI-Update-Server/update.json", true))) {
        u32 size = 0;
        if(R_SUCCEEDED(res = util_http_get_size(&context, &size))) {
            char* jsonText = (char*) calloc(sizeof(char), size);
            if(jsonText != NULL) {
                u32 bytesRead = 0;
                if(R_SUCCEEDED(res = util_http_read(&context, &bytesRead, (u8*) jsonText, size))) {
                    json_value* json = json_parse(jsonText, size);
                    if(json != NULL) {
                        if(json->type == json_object) {
                            
							// Store current version
							char currentVersion[16];
							snprintf(currentVersion, sizeof(currentVersion), "%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
							
							// Create 3 char to store version
							char major[3];
							char minor[3];
							char micro[3];
							
							// Read all the json to find major, minor and micro version
							for(u32 i = 0; i < json->u.object.length; i++) {
								// Create a json_value pointer that will be the value read from the json
                                json_value* val = json->u.object.values[i].value;
								
								// Create two variables that will store the values and it size
								char* name = json->u.object.values[i].name;
                                u32 nameLen = json->u.object.values[i].name_length;
								
								// If they aren't json_string, we don't need it
								if(val->type == json_string) {
									if(strncmp(name, "latest_major", nameLen) == 0) {
										// Found latest major										
										strncpy(major, val->u.string.ptr, sizeof(major));
									} else if(strncmp(name, "latest_minor", nameLen) == 0) {
										// Read latest minor
										strncpy(minor, val->u.string.ptr, sizeof(minor));
									} else if(strncmp(name, "latest_micro", nameLen) == 0) {
										// Read latest micro
										strncpy(micro, val->u.string.ptr, sizeof(micro));
									}
								}
							}
														
							// Store latest version
							char latestVersion[16];
							snprintf(latestVersion, sizeof(latestVersion), "%s.%s.%s", major, minor, micro);
							
							// Check if current version is the latest
							if(strncmp(currentVersion, latestVersion, sizeof(currentVersion)) != 0) {								
								char* url = NULL;
								
								for(u32 i = 0; i < json->u.object.length; i++) {
									// Create a json_value pointer that will find the download URL
									json_value* val = json->u.object.values[i].value;
									
									// Create two variables that will store the values and it size
									char* name = json->u.object.values[i].name;
									u32 nameLen = json->u.object.values[i].name_length;
									
									// If they aren't json_string, we don't need it
									if(val->type == json_string) {
										if(strncmp(name, "update_url", nameLen) == 0) {
											// Found update url!									
											strncpy(url, val->u.string.ptr, sizeof(major));
											break;
										}
									}
								}
								if(url != NULL) {
									strncpy(updateData->url, url, URL_MAX);
									hasUpdate = true;							
								} else {
									res = R_FBI_BAD_DATA;
								}
							}
							
                        } else {
                            res = R_FBI_BAD_DATA;
                        }
                    } else {
                        res = R_FBI_PARSE_FAILED;
                    }
                }

                free(jsonText);
            } else {
                res = R_FBI_OUT_OF_MEMORY;
            }
        }

        Result closeRes = util_http_close(&context);
        if(R_SUCCEEDED(res)) {
            res = closeRes;
        }
    }

    ui_pop();
    info_destroy(view);

    if(hasUpdate) {
        if(R_SUCCEEDED(res = task_data_op(&updateData->installInfo))) {
            info_display("Updating NTRDBI", "Press B to cancel.", true, data, update_install_update, NULL);
        } else {
            error_display_res(NULL, NULL, res, "Failed to begin update.");
        }
    } else {
        if(R_FAILED(res)) {
            if(res == R_FBI_HTTP_RESPONSE_CODE) {
                error_display(NULL, NULL, "Failed to check for update.\nHTTP server returned response code %d", responseCode);
            } else {
                error_display_res(NULL, NULL, res, "Failed to check for update.");
            }
        } else {
            prompt_display("Success", "No updates available.", COLOR_TEXT, false, NULL, NULL, NULL);
        }

        free(data);
    }
}

static void update_onresponse(ui_view* view, void* data, bool response) {
    if(response) {
        info_display("Checking For Updates", "", false, data, update_check_update, NULL);
    } else {
        free(data);
    }
}

void update_open() {
    update_data* data = (update_data*) calloc(1, sizeof(update_data));
    if(data == NULL) {
        error_display(NULL, NULL, "Failed to allocate update check data.");

        return;
    }

    data->responseCode = 0;

    data->installInfo.data = data;

    data->installInfo.op = DATAOP_COPY;

    data->installInfo.copyBufferSize = 128 * 1024;
    data->installInfo.copyEmpty = false;

    data->installInfo.total = 1;

    data->installInfo.isSrcDirectory = update_is_src_directory;
    data->installInfo.makeDstDirectory = update_make_dst_directory;

    data->installInfo.openSrc = update_open_src;
    data->installInfo.closeSrc = update_close_src;
    data->installInfo.getSrcSize = update_get_src_size;
    data->installInfo.readSrc = update_read_src;

    data->installInfo.openDst = update_open_dst;
    data->installInfo.closeDst = update_close_dst;
    data->installInfo.writeDst = update_write_dst;

    data->installInfo.suspendCopy = update_suspend_copy;
    data->installInfo.restoreCopy = update_restore_copy;

    data->installInfo.suspend = update_suspend;
    data->installInfo.restore = update_restore;

    data->installInfo.error = update_error;

    data->installInfo.finished = true;

    prompt_display("Confirmation", "Check for NTRDBI updates?", COLOR_TEXT, true, data, NULL, update_onresponse);
}
