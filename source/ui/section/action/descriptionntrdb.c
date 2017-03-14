#include <stdio.h>

#include <3ds.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include "action.h"
#include "descriptionntrdb.h"
#include "../task/task.h"
#include "../../list.h"
#include "../../prompt.h"
#include "../../ui.h"
#include "../../error.h"
#include "../../../core/screen.h"

void description_ntrdb_open(linked_list* items, list_item* selected) {
	ntrdb_info* data = (ntrdb_info*) selected->data;

    if(data->meta.description == NULL) {
        error_display(NULL, NULL, "Failed to read NTRDB description data.");

        return;
    }
	
    list_display(data->meta.name, "B: Return", NULL, ntrdb_update, ntrdb_draw_top);
}

static void ntrdb_update(ui_view* view, void* data, linked_list* items, list_item* selected, bool selectedTouched) {
    if(hidKeysDown() & KEY_B) {
        ui_pop();
        return;
    }
}

static void ntrdb_draw_top(ui_view* view, void* data, float x1, float y1, float x2, float y2, list_item* selected) {
	//ntrdb_info* info = (ntrdb_info*) selected->data;
	
	char str[] = "This cheat-menu includes:\r\n\r\n1. Text to Item\r\n2. Duplication\r\n3. Moon Jump\r\n4. Coordinates Modifier\r\n5. Teleport\r\n6. Speed Hack\r\n7. Seeder (Set, Destroy, Undo)\r\n8. Search and Replace\r\n9. Instant Tree\r\n10. Destroy All Weeds\r\n11. Water All Flowers\r\n12. Grass\r\n13. Desert\r\n14. Nookling Upgrades\r\n15. Tan Modifier\r\n16. Real Time World Edit\r\n17. Time Travel\r\n18. Time Machine ";

	int i;
	for(i = 0; str[i] != '\0'; i++) {
		if((str[i] == '\r') && (str[i + 1] == '\n')) {
			str[i] = '\n';
		}
	}
	for(i = 0; str[i] != '\0'; i++) {
		if((str[i] == '\n') && (str[i + 1] == '\n')) {
			str[i] = ' ';
		}
	}
	
	screen_draw_string(str, 8, 25, 0.5f, 0.5f, COLOR_TEXT, false);
}