#pragma once

#define FILE_NAME_MAX 512
#define FILE_PATH_MAX 512

typedef struct linked_list_s linked_list;
typedef struct list_item_s list_item;

typedef struct meta_info_s {
    char name[0x100];
	char compatible[0x10];
    char description[0x200];
    char developer[0x100];
	char devsite[0x100];
	char added[0x100];
    u32 texture;
	char version[0xA];
	char pic[0x100];
	u32 pictexture;
	u32 id;
} meta_info;

typedef struct ntrdb_info_s {
    char titleId[0xAA];
    meta_info meta;
	char downloadURL[0x100];
} ntrdb_info;

typedef enum data_op_e {
    DATAOP_COPY,
    DATAOP_DELETE
} data_op;

typedef struct data_op_data_s {
    void* data;

    data_op op;

    // Copy
    u32 copyBufferSize;
    bool copyEmpty;

    u32 copyBytesPerSecond;

    u32 processed;
    u32 total;

    u64 currProcessed;
    u64 currTotal;

    Result (*isSrcDirectory)(void* data, u32 index, bool* isDirectory);
    Result (*makeDstDirectory)(void* data, u32 index);

    Result (*openSrc)(void* data, u32 index, u32* handle);
    Result (*closeSrc)(void* data, u32 index, bool succeeded, u32 handle);

    Result (*getSrcSize)(void* data, u32 handle, u64* size);
    Result (*readSrc)(void* data, u32 handle, u32* bytesRead, void* buffer, u64 offset, u32 size);

    Result (*openFile)(void* data, u32 index, void* initialReadBlock, u64 size, u32* handle);
    Result (*closeFile)(void* data, u32 index, bool succeeded, u32 handle);

    Result (*writeFile)(void* data, u32 handle, u32* bytesWritten, void* buffer, u64 offset, u32 size);

    Result (*suspendCopy)(void* data, u32 index, u32* srcHandle, u32* dstHandle);
    Result (*restoreCopy)(void* data, u32 index, u32* srcHandle, u32* dstHandle);

    // Delete
    Result (*delete)(void* data, u32 index);

    // Suspend
    Result (*suspend)(void* data, u32 index);
    Result (*restore)(void* data, u32 index);

    // Errors
    bool (*error)(void* data, u32 index, Result res);

    // General
    volatile bool finished;
    Result result;
    Handle cancelEvent;
} data_op_data;

typedef struct populate_ntrdb_data_s {
    linked_list* items;

    volatile bool finished;
    Result result;
    Handle cancelEvent;
} populate_ntrdb_data;


typedef struct populate_titles_data_s {
    linked_list* items;

    void* userData;
    bool (*filter)(void* data, u64 titleId, FS_MediaType mediaType);
    int (*compare)(void* data, const void* p1, const void* p2);

    volatile bool finished;
    Result result;
    Handle cancelEvent;
} populate_titles_data;

typedef struct title_info_s {
    FS_MediaType mediaType;
    u64 titleId;
    char productCode[0x10];
    u16 version;
    u64 installedSize;
    bool twl;
    bool hasMeta;
    meta_info meta;
} title_info;

void task_init();
void task_exit();
bool task_is_quit_all();
Handle task_get_pause_event();
Handle task_get_suspend_event();

Result task_data_op(data_op_data* data);

void task_free_ntrdb(list_item* item);
void task_clear_ntrdb(linked_list* items);
Result task_populate_ntrdb(populate_ntrdb_data* data);
