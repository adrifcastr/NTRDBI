#include <stdio.h>

#include <3ds.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "action.h"
#include "install.h"
#include "../task/task.h"
#include "../../list.h"
#include "../../prompt.h"
#include "../../ui.h"
#include "../../../core/screen.h"


// Variables
char file[0x100];
char pluginURL[0x100];

void action_install_ntrdb(linked_list* items, list_item* selected) {
	internal_downloadPlugin_start("Download the selected plugin from NTRDB?", selected);
}

void internal_downloadPlugin_start(const char* text, list_item* selected) {
	// Read the plugin data first.
	ntrdb_info* ntrdbInfo = (ntrdb_info*) selected->data;

	// Store the download URL.
	char dwnULR[0x100];
	snprintf(dwnULR, sizeof(dwnULR), "%s", ntrdbInfo->downloadURL);
	
	if (strstr(dwnULR, "github") && !strstr(dwnULR, "?raw=true")) {
		snprintf(pluginURL, sizeof(pluginURL), "%s%s", dwnULR, "?raw=true");
	} else {
		snprintf(pluginURL, sizeof(pluginURL), "%s", dwnULR);
	}
	
	// Create the folder for plugin.
	char titleIDs[16]; // DEBUG
	snprintf(titleIDs, sizeof(titleIDs), "%s", ntrdbInfo->titleId);
	
	if (strstr(titleIDs, "No game")) 
		snprintf(titleIDs, sizeof(titleIDs), "%s", "game");
	
	// if dir already exist, mkdir won't do anything
	fsInit();
	char dir[50];
	snprintf(dir, sizeof(dir), "sdmc:/plugin/%s", titleIDs);
	dir[sizeof(dir) - 1] = '\0';
	mkdir(dir, 0777);
	fsExit();
	
	// DownloadFile wants file to be "/plugin/TID/name.plg"
	snprintf(file, sizeof(file), "/plugin/%s/%s.plg", titleIDs, ntrdbInfo->meta.name);

	// Show the confirmation text
	prompt_display("Confirmation", text, COLOR_TEXT, true, ntrdbInfo, NULL, internal_downloadPlugin_confirmed);
}

void internal_downloadPlugin_confirmed(ui_view* view, void* data, bool response) {	
	if(response) {
		info_display("Downloading plugin", "Press B to cancel.", false, data, internal_downloadPlugin_download, NULL);
	}
}

void internal_downloadPlugin_download(ui_view* view, void* data, float* progress, char* text){
	char* downloadURL = strdup(pluginURL);
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

	ui_pop();
	prompt_display("Success", "Download finished.", COLOR_TEXT, false, NULL, NULL, NULL);
	return;
}
