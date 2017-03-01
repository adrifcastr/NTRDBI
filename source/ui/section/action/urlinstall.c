#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "../action/action.h"
#include "../task/task.h"
#include "../../error.h"
#include "../../info.h"
#include "../../prompt.h"
#include "../../ui.h"
#include "../../../core/screen.h"
#include "../../../core/util.h"

#define URL_MAX 1024
#define URLS_MAX 128

typedef struct {
    char urls[URLS_MAX][URL_MAX];

    void* finishedData;
    void (*finished)(void* data);

    u32 responseCode;
    u64 currTitleId;
    data_op_data installInfo;
} url_install_data;

static void action_url_install_free_data(url_install_data* data) {
    if(data->finished != NULL) {
        data->finished(data->finishedData);
    }

    free(data);
}

static Result action_url_install_is_src_directory(void* data, u32 index, bool* isDirectory) {
    *isDirectory = false;
    return 0;
}

static Result action_url_install_make_dst_directory(void* data, u32 index) {
    return 0;
}

static Result action_url_install_open_src(void* data, u32 index, u32* handle) {
    url_install_data* installData = (url_install_data*) data;

    Result res = 0;

    httpcContext* context = (httpcContext*) calloc(1, sizeof(httpcContext));
    if(context != NULL) {
        if(R_SUCCEEDED(res = util_http_open(context, &installData->responseCode, installData->urls[index], true))) {
            *handle = (u32) context;
        } else {
            free(context);
        }
    } else {
        res = R_FBI_OUT_OF_MEMORY;
    }

    return res;
}

static Result action_url_install_close_src(void* data, u32 index, bool succeeded, u32 handle) {
    return util_http_close((httpcContext*) handle);
}

static Result action_url_install_get_src_size(void* data, u32 handle, u64* size) {
    u32 downloadSize = 0;
    Result res = util_http_get_size((httpcContext*) handle, &downloadSize);

    *size = downloadSize;
    return res;
}

static Result action_url_install_read_src(void* data, u32 handle, u32* bytesRead, void* buffer, u64 offset, u32 size) {
    return util_http_read((httpcContext*) handle, bytesRead, buffer, size);
}

static Result action_url_install_suspend_copy(void* data, u32 index, u32* srcHandle, u32* dstHandle) {
    return 0;
}

static Result action_url_install_restore_copy(void* data, u32 index, u32* srcHandle, u32* dstHandle) {
    return 0;
}

static Result action_url_install_suspend(void* data, u32 index) {
    return 0;
}

static Result action_url_install_restore(void* data, u32 index) {
    return 0;
}

static bool action_url_install_error(void* data, u32 index, Result res) {
    url_install_data* installData = (url_install_data*) data;

    if(res == R_FBI_CANCELLED) {
        prompt_display("Failure", "Install cancelled.", COLOR_TEXT, false, NULL, NULL, NULL);
        return false;
    } else if(res != R_FBI_WRONG_SYSTEM) {
        char* url = installData->urls[index];

        ui_view* view = NULL;

        if(res == R_FBI_HTTP_RESPONSE_CODE) {
            if(strlen(url) > 38) {
                view = error_display(NULL, NULL, "Failed to install from URL.\n%.35s...\nHTTP server returned response code %d", url, installData->responseCode);
            } else {
                view = error_display(NULL, NULL, "Failed to install from URL.\n%.38s\nHTTP server returned response code %d", url, installData->responseCode);
            }
        } else {
            if(strlen(url) > 38) {
                view = error_display_res(NULL, NULL, res, "Failed to install from URL.\n%.35s...", url);
            } else {
                view = error_display_res(NULL, NULL, res, "Failed to install from URL.\n%.38s", url);
            }
        }

        if(view != NULL) {
            svcWaitSynchronization(view->active, U64_MAX);
        }
    }

    return index < installData->installInfo.total - 1;
}

static void action_url_install_install_update(ui_view* view, void* data, float* progress, char* text) {
    url_install_data* installData = (url_install_data*) data;

    if(installData->installInfo.finished) {
        ui_pop();
        info_destroy(view);

        if(R_SUCCEEDED(installData->installInfo.result)) {
            prompt_display("Success", "Install finished.", COLOR_TEXT, false, NULL, NULL, NULL);
        }

        action_url_install_free_data(installData);

        return;
    }

    if(hidKeysDown() & KEY_B) {
        svcSignalEvent(installData->installInfo.cancelEvent);
    }

    *progress = installData->installInfo.currTotal != 0 ? (float) ((double) installData->installInfo.currProcessed / (double) installData->installInfo.currTotal) : 0;
    snprintf(text, PROGRESS_TEXT_MAX, "%lu / %lu\n%.2f %s / %.2f %s\n%.2f %s/s", installData->installInfo.processed, installData->installInfo.total, util_get_display_size(installData->installInfo.currProcessed), util_get_display_size_units(installData->installInfo.currProcessed), util_get_display_size(installData->installInfo.currTotal), util_get_display_size_units(installData->installInfo.currTotal), util_get_display_size(installData->installInfo.copyBytesPerSecond), util_get_display_size_units(installData->installInfo.copyBytesPerSecond));
}

static void action_url_install_confirm_onresponse(ui_view* view, void* data, bool response) {
    url_install_data* installData = (url_install_data*) data;

    if(response) {
        Result res = task_data_op(&installData->installInfo);
        if(R_SUCCEEDED(res)) {
            info_display("Installing From URL", "Press B to cancel.", true, data, action_url_install_install_update, NULL);
        } else {
            error_display_res(NULL, NULL, res, "Failed to initiate installation.");

            action_url_install_free_data(installData);
        }
    } else {
        action_url_install_free_data(installData);
    }
}

void action_url_install(const char* confirmMessage, const char* urls, void* finishedData, void (*finished)(void* data)) {
	/** This method will be used to download the .plg and store in the desired location. **/
	
    url_install_data* data = (url_install_data*) calloc(1, sizeof(url_install_data));
    if(data == NULL) {
        error_display(NULL, NULL, "Failed to allocate URL plugin data.");

        return;
    }

    data->installInfo.total = 0;

    size_t payloadLen = strlen(urls);
    if(payloadLen > 0) {
        const char* currStart = urls;
        while(data->installInfo.total < URLS_MAX && currStart - urls < payloadLen) {
            const char* currEnd = strchr(currStart, '\n');
            if(currEnd == NULL) {
                currEnd = urls + payloadLen;
            }

            u32 len = currEnd - currStart;

            if((len < 7 || strncmp(currStart, "http://", 7) != 0) && (len < 8 || strncmp(currStart, "https://", 8) != 0)) {
                if(len > URL_MAX - 7) {
                    len = URL_MAX - 7;
                }

                strncpy(data->urls[data->installInfo.total], "http://", 7);
                strncpy(&data->urls[data->installInfo.total][7], currStart, len);
            } else {
                if(len > URL_MAX) {
                    len = URL_MAX;
                }

                strncpy(data->urls[data->installInfo.total], currStart, len);
            }

            data->installInfo.total++;
            currStart = currEnd + 1;
        }
    }

    data->finishedData = finishedData;
    data->finished = finished;

    data->responseCode = 0;
    data->currTitleId = 0;

    data->installInfo.data = data;

    data->installInfo.op = DATAOP_COPY;

    data->installInfo.copyBufferSize = 128 * 1024;
    data->installInfo.copyEmpty = false;

    data->installInfo.isSrcDirectory = action_url_install_is_src_directory;
    data->installInfo.makeDstDirectory = action_url_install_make_dst_directory;

    data->installInfo.openSrc = action_url_install_open_src;
    data->installInfo.closeSrc = action_url_install_close_src;
    data->installInfo.getSrcSize = action_url_install_get_src_size;
    data->installInfo.readSrc = action_url_install_read_src;

    data->installInfo.suspendCopy = action_url_install_suspend_copy;
    data->installInfo.restoreCopy = action_url_install_restore_copy;

    data->installInfo.suspend = action_url_install_suspend;
    data->installInfo.restore = action_url_install_restore;

    data->installInfo.error = action_url_install_error;

    data->installInfo.finished = true;

    prompt_display("Confirmation", confirmMessage, COLOR_TEXT, true, data, NULL, action_url_install_confirm_onresponse);
}