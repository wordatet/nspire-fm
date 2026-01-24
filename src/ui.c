#include <nspireio/nspireio.h>
#include <libndls.h>
#include <string.h>
#include <stdio.h>
#include "fs.h"
#include "input.h"

// Constants
#define MAX_VISIBLE_ROWS 25
#define HEADER_height 1
#define FOOTER_height 1

/*
 * Format a file size into a human-readable string.
 * E.g., 1024 -> "1.0 KB", 1048576 -> "1.0 MB"
 */
static void format_file_size(unsigned int size, char *buf, size_t buf_size) {
    if (size < 1024) {
        snprintf(buf, buf_size, "%u B", size);
    } else if (size < 1024 * 1024) {
        snprintf(buf, buf_size, "%.1f KB", size / 1024.0);
    } else {
        snprintf(buf, buf_size, "%.1f MB", size / (1024.0 * 1024.0));
    }
}

/*
 * Draw the list of files and directories.
 *
 * Takes a file list, a selection index, and a scroll offset,
 * and draws the list of files and directories yanking them to the VRAM buffer.
 */

void ui_draw_list(file_list_t *list, int selection, int scroll_offset) {
    if (!list) return;

    nio_console *console = nio_get_default();
    nio_clear(console);

    // CRITICAL: Clear VRAM buffer to prevent artifacts (stuck selection lines)
    nio_vram_fill(0, 0, 320, 240, NIO_COLOR_BLACK);

    // 1. Draw Header (Current Path)
    nio_color(console, NIO_COLOR_BLUE, NIO_COLOR_WHITE);
    // 1. Draw Header (Current Path)
    nio_color(console, NIO_COLOR_BLUE, NIO_COLOR_WHITE);
    // Fill header line
    nio_vram_fill(0, 0, 320, 10, NIO_COLOR_BLUE);
    // Center text vertically (offset_y=1)
    nio_vram_grid_puts(0, 1, 0, 0, list->path, NIO_COLOR_BLUE, NIO_COLOR_WHITE);

    // 2. Draw List
    nio_color(console, NIO_COLOR_BLACK, NIO_COLOR_WHITE);
    
    int list_y_start = 1; 

    for (int i = 0; i < MAX_VISIBLE_ROWS; i++) {
        int entry_idx = scroll_offset + i;
        if (entry_idx >= list->count) break;
        
        file_entry_t *entry = &list->entries[entry_idx];
        
        int is_selected = (entry_idx == selection);
        
        // Pixel y position of the row (Row 1 * 8 + 2px offset = 10px start)
        int row_y_px = (list_y_start + i) * 8 + 2;

        // Setup colors for this row
        if (is_selected) {
             nio_color(console, NIO_COLOR_CYAN, NIO_COLOR_BLACK);
             // Fill selection background
             nio_vram_fill(0, row_y_px, 320, 8, NIO_COLOR_CYAN);
        } else {
             nio_color(console, NIO_COLOR_BLACK, NIO_COLOR_WHITE);
        }
        
        // Construct line with name and size - consistent format for all entries
        // Format: "[icon] [name padded to 25 chars] [size/type padded to 8 chars]"
        char line[64];
        char size_str[16] = "";
        
        if (entry->is_dir) {
            if (strcmp(entry->name, "..") == 0) {
                snprintf(line, sizeof(line), "/ %-25s %8s", "..", "<UP>");
            } else {
                snprintf(line, sizeof(line), "/ %-25s %8s", entry->name, "<DIR>");
            }
        } else {
            format_file_size(entry->size, size_str, sizeof(size_str));
            snprintf(line, sizeof(line), "  %-25s %8s", entry->name, size_str);
        }
        
        // Use grid put with offset_y=2 to clear the header
        nio_vram_grid_puts(0, 2, 0, list_y_start + i, line, 
                           is_selected ? NIO_COLOR_CYAN : NIO_COLOR_BLACK, 
                           is_selected ? NIO_COLOR_BLACK : NIO_COLOR_WHITE);
    }
    
    // 3. Draw Footer (Instructions + Page indicator)
    int footer_y = 29; // Approx bottom
    
    // Calculate page info
    int total_pages = (list->count + MAX_VISIBLE_ROWS - 1) / MAX_VISIBLE_ROWS;
    int current_page = (scroll_offset / MAX_VISIBLE_ROWS) + 1;
    if (total_pages < 1) total_pages = 1;
    
    char footer_text[64];
    snprintf(footer_text, sizeof(footer_text), "CTRL:Menu ENTER:Open Q:Exit  [%d/%d]", current_page, total_pages);
    
    // Fill footer
    nio_vram_fill(0, footer_y * 8, 320, 8, NIO_COLOR_GRAY);
    nio_vram_grid_puts(0, 0, 0, footer_y, footer_text, NIO_COLOR_GRAY, NIO_COLOR_BLACK);
    
    // Force draw
    nio_vram_draw();
}

/*
 * Takes a message string and draws a modal dialog with the message and
 * two options ("Yes" and "No") in the VRAM buffer. Very helpful for
 * confirmation dialogs.
 */
 
void ui_draw_modal(const char *msg) {
    // Calculate required width based on text length
    // Assuming approx 8px per char (standard font width)
    int text_len = strlen(msg);
    int char_width = 8;
    int padding = 20;
    int required_w = (text_len * char_width) + (padding * 2);
    
    // Clamp width
    int w = required_w;
    if (w < 200) w = 200;
    if (w > 300) w = 300; // max width
    
    int h = 60;
    int x = (320 - w) / 2;
    int y = (240 - h) / 2;
    
    // Draw Border (Black)
    nio_vram_fill(x - 2, y - 2, w + 4, h + 4, NIO_COLOR_BLACK);
    // Draw Body (White)
    nio_vram_fill(x, y, w, h, NIO_COLOR_WHITE);
    
    // Draw Text - Centered
    int text_pixel_width = text_len * char_width;
    int text_x = x + (w - text_pixel_width) / 2;
    // Ensure text doesn't start before left padding if clamped
    if (text_x < x + 5) text_x = x + 5; 
    
    nio_vram_grid_puts(text_x, y + 20, 0, 0, msg, NIO_COLOR_WHITE, NIO_COLOR_BLACK);
    
    nio_vram_draw();
}

/*
 * Draw a menu with a list of options.
 *
 * Takes an array of option strings, the number of options, and the current selection index,
 * and draws a menu with the options in the VRAM buffer.
 */
 
void ui_draw_menu(const char **options, int count, int selection) {
    // Menu Dimensions
    int item_height = 10; 
    int w = 120;
    int h = (count * item_height) + 10; // Padding
    int x = (320 - w) / 2;
    int y = (240 - h) / 2;
    
    // Draw Shadow/Border
    nio_vram_fill(x + 4, y + 4, w, h, NIO_COLOR_BLACK); // Simple shadow
    nio_vram_fill(x - 1, y - 1, w + 2, h + 2, NIO_COLOR_BLACK); // Border
    nio_vram_fill(x, y, w, h, NIO_COLOR_WHITE); // Body
    
    for (int i = 0; i < count; i++) {
        int is_sel = (i == selection);
        int item_y_px = y + 5 + (i * item_height);
        
        if (is_sel) {
            nio_vram_fill(x, item_y_px, w, item_height, NIO_COLOR_BLUE);
        }
        
        // Draw text
        nio_vram_grid_puts(x + 5, item_y_px + 1, 0, 0, options[i], 
                          is_sel ? NIO_COLOR_BLUE : NIO_COLOR_WHITE, 
                          is_sel ? NIO_COLOR_WHITE : NIO_COLOR_BLACK);
    }
    
    nio_vram_draw();
}

/*
 * Get a string from the user.
 *
 * Takes a prompt string and a buffer to store the input string,
 * and returns the length of the input string.
 */

int ui_get_string(const char *prompt, char *buffer, int max_len) {
    int len = strlen(buffer);
    
    // Box
    int w = 240;
    int h = 60;
    int x = (320 - w) / 2;
    int y = (240 - h) / 2;
    
    while (1) {
        // Draw Box
        nio_vram_fill(x - 2, y - 2, w + 4, h + 4, NIO_COLOR_BLACK);
        nio_vram_fill(x, y, w, h, NIO_COLOR_WHITE);
        
        // Prompt
        nio_vram_grid_puts(x + 10, y + 10, 0, 0, prompt, NIO_COLOR_WHITE, NIO_COLOR_BLACK);
        
        // Input Field Background
        nio_vram_fill(x + 10, y + 30, w - 20, 14, NIO_COLOR_GRAY); 
        
        nio_vram_grid_puts(x + 12, y + 32, 0, 0, buffer, NIO_COLOR_WHITE /*bg of box*/, NIO_COLOR_BLACK);
        
        // Cursor (6px char width for nspireio font)
        int cursor_x = x + 12 + (len * 6);
        if (cursor_x > 318) cursor_x = 318; // Clip to screen edge
        nio_vram_fill(cursor_x, y + 32, 2, 10, NIO_COLOR_BLACK);
        
        nio_vram_draw();
        
        // Input
        int c = input_get_key();
        
        if (c == NIO_KEY_ESC) {
            return 0;
        } else if (c == NIO_KEY_ENTER) {
            return 1;
        } else if (c == 8 || c == 0x7F) { // Backspace
            if (len > 0) {
                buffer[--len] = '\0';
            }
        } else if (c >= 32 && c <= 126) { // Printable
            // VISUAL SAFETY LIMIT: 30 chars max to prevent drawing off-screen
            int limit = 30;
            if (limit > max_len - 1) limit = max_len - 1;
            
            if (len < limit) {
                buffer[len++] = (char)c;
                buffer[len] = '\0';
            }
        }
    }
}

int ui_get_confirmation(const char *msg) {
    int selected = 1; // Default to Yes
    
    // Calculate box width dynamically
    int text_len = strlen(msg);
    int char_width = 8;
    int padding = 20;
    int w = (text_len * char_width) + (padding * 2);
    if (w < 200) w = 200;
    if (w > 300) w = 300;
    
    int h = 80;
    int x = (320 - w) / 2;
    int y = (240 - h) / 2;
    
    while(1) {
        // Draw Box
        nio_vram_fill(x - 2, y - 2, w + 4, h + 4, NIO_COLOR_BLACK);
        nio_vram_fill(x, y, w, h, NIO_COLOR_WHITE);
        
        // Draw Message - Centered
        int text_pixel_width = text_len * char_width;
        int text_x = x + (w - text_pixel_width) / 2;
        if (text_x < x + 5) text_x = x + 5;
        nio_vram_grid_puts(text_x, y + 15, 0, 0, msg, NIO_COLOR_WHITE, NIO_COLOR_BLACK);
        
        // Draw Buttons
        // Yes Button
        int btn_w = 60;
        int btn_h = 15;
        int yes_x = x + (w/4) - (btn_w/2);
        int no_x = x + (3*w/4) - (btn_w/2);
        int btn_y = y + 50;
        
        // Yes
        if (selected == 1) {
            nio_vram_fill(yes_x, btn_y, btn_w, btn_h, NIO_COLOR_BLUE); // Highlighted
            nio_vram_grid_puts(yes_x + 15, btn_y + 2, 0, 0, "Yes", NIO_COLOR_BLUE, NIO_COLOR_WHITE);
        } else {
            nio_vram_fill(yes_x - 1, btn_y - 1, btn_w + 2, btn_h + 2, NIO_COLOR_BLACK); // Border
            nio_vram_fill(yes_x, btn_y, btn_w, btn_h, NIO_COLOR_WHITE); // Normal Body
            nio_vram_grid_puts(yes_x + 15, btn_y + 2, 0, 0, "Yes", NIO_COLOR_WHITE, NIO_COLOR_BLACK);
        }
        
        // No
        if (selected == 0) {
            nio_vram_fill(no_x, btn_y, btn_w, btn_h, NIO_COLOR_BLUE); // Highlighted
            nio_vram_grid_puts(no_x + 20, btn_y + 2, 0, 0, "No", NIO_COLOR_BLUE, NIO_COLOR_WHITE);
        } else {
            nio_vram_fill(no_x - 1, btn_y - 1, btn_w + 2, btn_h + 2, NIO_COLOR_BLACK); // Border
            nio_vram_fill(no_x, btn_y, btn_w, btn_h, NIO_COLOR_WHITE); // Normal Body
            nio_vram_grid_puts(no_x + 20, btn_y + 2, 0, 0, "No", NIO_COLOR_WHITE, NIO_COLOR_BLACK);
        }
        
        nio_vram_draw();
        
        int k = input_get_key();
        if (k == NIO_KEY_LEFT || k == NIO_KEY_RIGHT) {
            selected = !selected;
        } else if (k == NIO_KEY_ENTER) {
            return selected;
        } else if (k == NIO_KEY_ESC || k == 'n') {
            return 0;
        } else if (k == 'y') {
            return 1;
        }
    }
}
