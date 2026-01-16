#include <nspireio/nspireio.h>
#include <libndls.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor.h"
#include "input.h"
#include "ui.h"


/*
 * Buffer limits: 256 lines * 128 chars = 32KB max file size.
 * Fits comfortably in Nspire's limited RAM while allowing most scripts.
 */

#define MAX_LINES 256
#define MAX_LINE_LEN 128

#define VISIBLE_ROWS 26

typedef struct {
    char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count;
    int cursor_line;
    int cursor_col;
    int scroll_offset;
    int modified;
} editor_state_t;

/*
 * Text Editor Module
 *
 * Simple text editor for .txt, .py, .lua, .xml, .csv files.
 * Uses a line-based buffer with cursor navigation, insert,
 * delete, and newline support. Scroll support for long files.
 *
 * Controls: Arrows=Navigate, Enter=Newline, Backspace=Delete,
 *           Ctrl/Menu=Save, Esc=Exit
 */

static void editor_draw(editor_state_t *e, const char *title) {
    nio_console *console = nio_get_default();
    nio_clear(console);
    
    // Header
    nio_vram_fill(0, 0, 320, 10, NIO_COLOR_BLUE);
    nio_vram_grid_puts(0, 0, 0, 0, title, NIO_COLOR_BLUE, NIO_COLOR_WHITE);
    
    if (e->modified) {
        nio_vram_grid_puts(280, 0, 0, 0, "[*]", NIO_COLOR_BLUE, NIO_COLOR_YELLOW);
    }
    
    // Text area
    for (int i = 0; i < VISIBLE_ROWS && (i + e->scroll_offset) < e->line_count; i++) {
        int line_idx = i + e->scroll_offset;
        int y = 12 + (i * 8);
        
        // Highlight current line
        if (line_idx == e->cursor_line) {
            nio_vram_fill(0, y, 320, 8, NIO_COLOR_GRAY);
        }
        
        nio_vram_grid_puts(0, y, 0, 0, e->lines[line_idx], 
                          (line_idx == e->cursor_line) ? NIO_COLOR_GRAY : NIO_COLOR_WHITE, 
                          NIO_COLOR_BLACK);
    }
    
    // Cursor (simple block)
    int cursor_y = 12 + ((e->cursor_line - e->scroll_offset) * 8);
    int cursor_x = e->cursor_col * 6; // Approx char width
    if (cursor_x > 310) cursor_x = 310;
    nio_vram_fill(cursor_x, cursor_y, 2, 8, NIO_COLOR_BLACK);
    
    // Footer
    nio_vram_fill(0, 230, 320, 10, NIO_COLOR_GRAY);
    nio_vram_grid_puts(0, 231, 0, 0, "Ctrl:Save  Esc:Exit", NIO_COLOR_GRAY, NIO_COLOR_WHITE);
    
    nio_vram_draw();
}

/*
 * Load file into editor state. We do sanity checks and 
 * initialize the editor state, sanitize the file, and 
 * load the file into the editor state. After that we 
 * return 1 on success and 0 on failure.
 */
static int editor_load(editor_state_t *e, const char *filepath) {
    memset(e, 0, sizeof(*e));
    e->line_count = 1; // Start with one empty line
    
    FILE *f = fopen(filepath, "r");
    if (!f) {
        return 0; // New file
    }
    
    e->line_count = 0;
    char buf[MAX_LINE_LEN];
    while (fgets(buf, sizeof(buf), f) && e->line_count < MAX_LINES) {
        // Strip newline
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') buf[len-1] = '\0';
        if (len > 1 && buf[len-2] == '\r') buf[len-2] = '\0';
        
        strncpy(e->lines[e->line_count], buf, MAX_LINE_LEN - 1);
        e->line_count++;
    }
    
    fclose(f);
    
    if (e->line_count == 0) e->line_count = 1;
    return 1;
}

/*
 * Save the file to disk by writing each line to the file.
 * We also reset the modified flag and return 1 on success
 */

static int editor_save(editor_state_t *e, const char *filepath) {
    FILE *f = fopen(filepath, "w");
    if (!f) return 0;
    
    for (int i = 0; i < e->line_count; i++) {
        fputs(e->lines[i], f);
        fputc('\n', f);
    }
    
    fclose(f);
    e->modified = 0;
    return 1;
}

/*
 * Open the file in the editor.
 *
 * We load the file into the editor state, extract the filename for the title,
 * and enter the main loop where we handle user input and update the editor state.
 */
int editor_open(const char *filepath) {
    editor_state_t e;
    editor_load(&e, filepath);
    
    // Extract filename for title
    const char *title = strrchr(filepath, '/');
    if (title) title++; else title = filepath;
    
    while (1) {
        editor_draw(&e, title);
        
        int c = input_get_key();
        
        if (c == NIO_KEY_ESC) {
            // Exit (discard changes)
            return 0;
        } else if (c == NIO_KEY_MENU) {
            // Save (Ctrl/Menu = Save)
            if (ui_get_confirmation("Save changes?")) {
                if (editor_save(&e, filepath)) {
                    ui_draw_modal("Saved!");
                    wait_key_pressed();
                    wait_no_key_pressed();
                    return 1;
                }
            }
        } else if (c == NIO_KEY_UP) {
            if (e.cursor_line > 0) {
                e.cursor_line--;
                if (e.cursor_line < e.scroll_offset) e.scroll_offset--;
                if (e.cursor_col > (int)strlen(e.lines[e.cursor_line]))
                    e.cursor_col = strlen(e.lines[e.cursor_line]);
            }
        } else if (c == NIO_KEY_DOWN) {
            if (e.cursor_line < e.line_count - 1) {
                e.cursor_line++;
                if (e.cursor_line >= e.scroll_offset + VISIBLE_ROWS) e.scroll_offset++;
                if (e.cursor_col > (int)strlen(e.lines[e.cursor_line]))
                    e.cursor_col = strlen(e.lines[e.cursor_line]);
            }
        } else if (c == NIO_KEY_LEFT) {
            if (e.cursor_col > 0) {
                e.cursor_col--;
            } else if (e.cursor_line > 0) {
                e.cursor_line--;
                e.cursor_col = strlen(e.lines[e.cursor_line]);
            }
        } else if (c == NIO_KEY_RIGHT) {
            if (e.cursor_col < (int)strlen(e.lines[e.cursor_line])) {
                e.cursor_col++;
            } else if (e.cursor_line < e.line_count - 1) {
                e.cursor_line++;
                e.cursor_col = 0;
            }
        } else if (c == NIO_KEY_ENTER) {
            // Insert new line
            if (e.line_count < MAX_LINES) {
                // Shift lines down
                for (int i = e.line_count; i > e.cursor_line + 1; i--) {
                    strcpy(e.lines[i], e.lines[i-1]);
                }
                e.line_count++;
                
                // Split current line
                char *cur = e.lines[e.cursor_line];
                strcpy(e.lines[e.cursor_line + 1], cur + e.cursor_col);
                cur[e.cursor_col] = '\0';
                
                e.cursor_line++;
                e.cursor_col = 0;
                e.modified = 1;
            }
        } else if (c == 8 || c == 0x7F) { // Backspace
            if (e.cursor_col > 0) {
                char *cur = e.lines[e.cursor_line];
                memmove(cur + e.cursor_col - 1, cur + e.cursor_col, strlen(cur + e.cursor_col) + 1);
                e.cursor_col--;
                e.modified = 1;
            } else if (e.cursor_line > 0) {
                // Merge with previous line
                int prev_len = strlen(e.lines[e.cursor_line - 1]);
                int cur_len = strlen(e.lines[e.cursor_line]);
                if (prev_len + cur_len < MAX_LINE_LEN - 1) {
                    // Use temp buffer to avoid overlap
                    char temp[MAX_LINE_LEN];
                    strcpy(temp, e.lines[e.cursor_line]);
                    strcat(e.lines[e.cursor_line - 1], temp);
                    
                    // Shift lines up
                    for (int i = e.cursor_line; i < e.line_count - 1; i++) {
                        strcpy(e.lines[i], e.lines[i+1]);
                    }
                    e.line_count--;
                    e.cursor_line--;
                    e.cursor_col = prev_len;
                    e.modified = 1;
                }
            }
        } else if (c >= 32 && c <= 126) { // Printable
            char *cur = e.lines[e.cursor_line];
            int len = strlen(cur);
            if (len < MAX_LINE_LEN - 2) {
                memmove(cur + e.cursor_col + 1, cur + e.cursor_col, len - e.cursor_col + 1);
                cur[e.cursor_col] = (char)c;
                e.cursor_col++;
                e.modified = 1;
            }
        }
    }
}
