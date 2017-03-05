#include <stdio.h>

#include <3ds.h>

#include "action.h"
#include "installtitledb.h"
#include "../task/task.h"
#include "../../list.h"
#include "../../prompt.h"
#include "../../../core/screen.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

// Variables
char file[0x100];
char pluginURL[0x100];

void action_install_titledb(linked_list* items, list_item* selected) {
    char url[0x100];
    snprintf(url, sizeof(url), "%s", ((titledb_info*) selected->data)->downloadURL);
    downloadPlugin("Install the selected title from NTRDB?", url, selected);
}

void downloadPlugin(const char* text, const char* url, list_item* selected) {
	/** This method will be used to download the .plg and store in the desired location. **/

	// Read the plugin data first.
	titledb_info* titledbInfo = (titledb_info*) selected->data;

	// Create the folder for plugin.
	u64 titleID = titledbInfo->titleId;
	char titleID_s[] = {"0123456789ABCDEF"}; // dummy titleID. TODO check if this titleID folder is generated. if true, there is an error!
	snprintf(titleID_s, sizeof(titleID_s), "%016llX", titleID);
	
	// if dir already exist, mkdir won't do anything
	fsInit();
	char dir[50];
	snprintf(dir, sizeof(dir), "sdmc:/plugin/%s", titleID_s);
	dir[49] = '\0';
	mkdir(dir, 0777);
	fsExit();
	
	// DownloadFile wants file to be "/plugins/TID/name"
	char tmp[0x100];
	//snprintf(tmp, sizeof(tmp), "/plugin/%s/%s", titleID_s, titledbInfo->meta.shortDescription);
	snprintf(tmp, sizeof(tmp), "/plugin/%s/%s.plg", titleID_s, titledbInfo->meta.shortDescription);

	// Store
	///strncpy(file, tmp, sizeof(titleID));
	///strncpy(pluginURL, url, sizeof(pluginURL));
	///file[255] = '\0';

	// Download: TODO fix the code above this to make all UI WORKS!!. This is a TEMP solution!
	
	char* downloadURL = strdup(url);
	fsInit();
	httpcInit(0x1000);
	httpcContext context;
	u32 statuscode=0;
	HTTPC_RequestMethod useMethod = HTTPC_METHOD_GET;

	do {
		if (statuscode >= 301 && statuscode <= 308) {
			char newurl[4096];
			httpcGetResponseHeader(&context, (char*)"Location", &newurl[0], 4096);
			downloadURL = &newurl[0];
			httpcCloseContext(&context);
		}

		Result ret = httpcOpenContext(&context, useMethod, downloadURL, 0);
		httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);

		if (ret==0) {
			if(R_SUCCEEDED(httpcBeginRequest(&context))){
				u32 contentsize=0;
				if(R_FAILED(httpcGetResponseStatusCode(&context, &statuscode))){
					httpcCloseContext(&context);
					httpcExit();
					fsExit();
					return;
				}
				if (statuscode == 200) {
					u32 readSize = 0;
					long int bytesWritten = 0;

					Handle fileHandle;
					FS_Path filePath=fsMakePath(PATH_ASCII, tmp);
					FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), filePath, FS_OPEN_CREATE | FS_OPEN_WRITE, 0x00000000);

					if(R_FAILED(httpcGetDownloadSizeState(&context, NULL, &contentsize))){
						httpcCloseContext(&context);
						httpcExit();
						fsExit();
						return;
					}
					u8* buf = (u8*)malloc(contentsize);
					memset(buf, 0, contentsize);

					do {
						if(R_FAILED(ret = httpcDownloadData(&context, buf, contentsize, &readSize))){
							free(buf);
							httpcCloseContext(&context);
							httpcExit();
							fsExit();
							return;
						}
						FSFILE_Write(fileHandle, NULL, bytesWritten, buf, readSize, 0x10001);
						bytesWritten += readSize;
					} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

					FSFILE_Close(fileHandle);
					svcCloseHandle(fileHandle);

					free(buf);
				}
			}else{
				httpcCloseContext(&context);
				httpcExit();
				fsExit();
				return;
			}
		}else{
			httpcCloseContext(&context);
			httpcExit();
			fsExit();
			return;
		}
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));
	httpcCloseContext(&context);

	httpcExit();
	fsExit();
	
	/// Show the confirmation text
	///prompt_display("Confirmation", text, COLOR_TEXT, true, (titledb_info*) selected->data, NULL, downloadPlugin_confirmed);
}
/*
void downloadPlugin_confirmed(ui_view* view, void* data, bool response) {
   info_display("Downloading plugin", "Press B to cancel.", false, data, startDownload, NULL);
}

void startDownload(ui_view* view, void* data, float* progress, char* text){

	char* downloadURL = pluginURL;
	fsInit();
	httpcInit(0x1000);
	httpcContext context;
	u32 statuscode=0;
	HTTPC_RequestMethod useMethod = HTTPC_METHOD_GET;

	do {
		if (statuscode >= 301 && statuscode <= 308) {
			char newurl[4096];
			httpcGetResponseHeader(&context, (char*)"Location", &newurl[0], 4096);
			downloadURL = &newurl[0];
			httpcCloseContext(&context);
		}

		Result ret = httpcOpenContext(&context, useMethod, downloadURL, 0);
		httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);

		if (ret==0) {
			if(R_SUCCEEDED(httpcBeginRequest(&context))){
				u32 contentsize=0;
				if(R_FAILED(httpcGetResponseStatusCode(&context, &statuscode))){
					httpcCloseContext(&context);
					httpcExit();
					fsExit();
					return;
				}
				if (statuscode == 200) {
					u32 readSize = 0;
					long int bytesWritten = 0;

					Handle fileHandle;
					FS_Path filePath=fsMakePath(PATH_ASCII, file);
					FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""), filePath, FS_OPEN_CREATE | FS_OPEN_WRITE, 0x00000000);

					if(R_FAILED(httpcGetDownloadSizeState(&context, NULL, &contentsize))){
						httpcCloseContext(&context);
						httpcExit();
						fsExit();
						return;
					}
					u8* buf = (u8*)malloc(contentsize);
					memset(buf, 0, contentsize);

					do {
						if(R_FAILED(ret = httpcDownloadData(&context, buf, contentsize, &readSize))){
							free(buf);
							httpcCloseContext(&context);
							httpcExit();
							fsExit();
							return;
						}
						FSFILE_Write(fileHandle, NULL, bytesWritten, buf, readSize, 0x10001);
						bytesWritten += readSize;
					} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

					FSFILE_Close(fileHandle);
					svcCloseHandle(fileHandle);

					free(buf);
				}
			}else{
				httpcCloseContext(&context);
				httpcExit();
				fsExit();
				return;
			}
		}else{
			httpcCloseContext(&context);
			httpcExit();
			fsExit();
			return;
		}
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));
	httpcCloseContext(&context);

	httpcExit();
	fsExit();
	info_destroy(view);

	prompt_display("Success", "Download finished.", COLOR_TEXT, false, NULL, NULL, NULL);
	return;
}*/
