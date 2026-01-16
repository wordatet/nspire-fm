#ifndef UI_H
#define UI_H

#include "fs.h"

void ui_draw_list(file_list_t *list, int selection, int scroll_offset);
void ui_draw_modal(const char *msg);
void ui_draw_menu(const char **options, int count, int selection);
int ui_get_string(const char *prompt, char *buffer, int max_len);

// Returns 1 for Yes, 0 for No/Esc
int ui_get_confirmation(const char *msg);

#endif
