/*
 * Hex Viewer
 *
 * Read-only hex dump viewer for binary files.
 * Displays offset, hex bytes, and ASCII representation.
 * If the a character is not printable, it is displayed as a dot.
 *
 * Controls: Up/Down=Line scroll, Left/Right=Page scroll, Esc=Exit
 */

#include <nspireio/nspireio.h>
#include <libndls.h>
#include <stdio.h>
#include <string.h>
#include "viewer.h"
#include "ui.h"
#include "input.h"

#define BYTES_PER_LINE 8
#define VISIBLE_LINES 25

// Helper from ui.c if we wanted to share, but for now we'll do a simple local version
// to avoid linker complexity if ui.c changes.
static void format_size_local(unsigned int size, char *buf, size_t buf_size) {
    if (size < 1024) {
        snprintf(buf, buf_size, "%u B", size);
    } else if (size < 1024 * 1024) {
        snprintf(buf, buf_size, "%.1f KB", size / 1024.0);
    } else {
        snprintf(buf, buf_size, "%.1f MB", size / (1024.0 * 1024.0));
    }
}

/*
 * Logic for the hex dump.
 *
 * Takes a file pointer, an offset, a file size, and a title,
 * and draws the hex dump in the VRAM buffer.
 */
 
static void viewer_draw(FILE *f, long offset, long file_size, const char *title) {
    nio_console *console = nio_get_default();
    nio_clear(console);
    
    // Header
    nio_vram_fill(0, 0, 320, 10, NIO_COLOR_MAGENTA);
    nio_vram_grid_puts(0, 0, 0, 0, title, NIO_COLOR_MAGENTA, NIO_COLOR_WHITE);
    
    // Offset info
    char info[64];
    char size_buf[32];
    format_size_local(file_size, size_buf, sizeof(size_buf));
    snprintf(info, sizeof(info), "%08lX/%08lX (%s)", offset, file_size, size_buf);
    nio_vram_grid_puts(120, 0, 0, 0, info, NIO_COLOR_MAGENTA, NIO_COLOR_WHITE);
    
    // Seek to offset
    fseek(f, offset, SEEK_SET);
    
    // Draw hex dump
    for (int line = 0; line < VISIBLE_LINES; line++) {
        int y = 12 + (line * 8);
        long line_offset = offset + (line * BYTES_PER_LINE);
        
        if (line_offset >= file_size) break;
        
        // Offset column (8-digit) - Starts at col 0
        char addr[16];
        snprintf(addr, sizeof(addr), "%08lX:", line_offset);
        nio_vram_grid_puts(0, y, 0, 0, addr, NIO_COLOR_WHITE, NIO_COLOR_BLUE);
        
        // Read bytes
        unsigned char buf[BYTES_PER_LINE];
        int bytes_read = fread(buf, 1, BYTES_PER_LINE, f);
        
        // Hex column (8 bytes) - Starts at col 10 (60px)
        char hex[64] = "";
        for (int i = 0; i < bytes_read; i++) {
            char byte_hex[4];
            snprintf(byte_hex, sizeof(byte_hex), "%02X ", buf[i]);
            strcat(hex, byte_hex);
        }
        nio_vram_grid_puts(60, y, 0, 0, hex, NIO_COLOR_WHITE, NIO_COLOR_GREEN);
        
        // ASCII column - Starts at col 36 (216px)
        char ascii[BYTES_PER_LINE + 1];
        for (int i = 0; i < bytes_read; i++) {
            ascii[i] = (buf[i] >= 32 && buf[i] <= 126) ? buf[i] : '.';
        }
        ascii[bytes_read] = '\0';
        nio_vram_grid_puts(216, y, 0, 0, ascii, NIO_COLOR_WHITE, NIO_COLOR_CYAN);
    }
    
    // Footer
    nio_vram_fill(0, 230, 320, 10, NIO_COLOR_GRAY);
    nio_vram_grid_puts(0, 231, 0, 0, "Up/Down:Line L/R:Page G:Goto Esc:Exit", NIO_COLOR_GRAY, NIO_COLOR_WHITE);
    
    nio_vram_draw();
}

/*
 * Opens a file in the hex viewer.
 *
 * Takes a file path and opens it in the hex viewer. Inspired
 * by xxd.
 */

void viewer_open(const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) return;
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Extract filename for title
    const char *title = strrchr(filepath, '/');
    if (title) title++; else title = filepath;
    
    long offset = 0;
    long page_size = BYTES_PER_LINE * VISIBLE_LINES;
    
    while (1) {
        viewer_draw(f, offset, file_size, title);
        
        int c = input_get_key();
        
        if (c == NIO_KEY_ESC) {
            break;
        } else if (c == NIO_KEY_UP) {
            if (offset >= BYTES_PER_LINE) {
                offset -= BYTES_PER_LINE;
            }
        } else if (c == NIO_KEY_DOWN) {
            if (offset + BYTES_PER_LINE < file_size) {
                offset += BYTES_PER_LINE;
            }
        } else if (c == NIO_KEY_LEFT) {
            // Page up
            if (offset >= page_size) {
                offset -= page_size;
            } else {
                offset = 0;
            }
        } else if (c == NIO_KEY_RIGHT) {
            // Page down
            if (offset + page_size < file_size) {
                offset += page_size;
            }
        } else if (c == 'g' || c == 'G') {
            char input_buf[16] = "";
            if (ui_get_string("Go to offset (hex):", input_buf, sizeof(input_buf))) {
                long new_offset = strtol(input_buf, NULL, 16);
                if (new_offset < 0) new_offset = 0;
                if (new_offset >= file_size) new_offset = file_size - 1;
                if (new_offset < 0) new_offset = 0; // Handle empty files
                
                // Align to line
                offset = (new_offset / BYTES_PER_LINE) * BYTES_PER_LINE;
            }
        }
    }
    fclose(f);
}
