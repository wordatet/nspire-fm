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
#include "input.h"

#define BYTES_PER_LINE 8
#define VISIBLE_LINES 25


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
    char info[32];
    snprintf(info, sizeof(info), "%04lX/%04lX", offset, file_size);
    nio_vram_grid_puts(260, 0, 0, 0, info, NIO_COLOR_MAGENTA, NIO_COLOR_WHITE);
    
    // Seek to offset
    fseek(f, offset, SEEK_SET);
    
    // Draw hex dump
    for (int line = 0; line < VISIBLE_LINES; line++) {
        int y = 12 + (line * 8);
        long line_offset = offset + (line * BYTES_PER_LINE);
        
        if (line_offset >= file_size) break;
        
        // Offset column
        char addr[16];
        snprintf(addr, sizeof(addr), "%04lX:", line_offset);
        nio_vram_grid_puts(0, y, 0, 0, addr, NIO_COLOR_WHITE, NIO_COLOR_BLUE);
        
        // Read bytes
        unsigned char buf[BYTES_PER_LINE];
        int bytes_read = fread(buf, 1, BYTES_PER_LINE, f);
        
        // Hex column
        char hex[64] = "";
        for (int i = 0; i < bytes_read; i++) {
            char byte_hex[4];
            snprintf(byte_hex, sizeof(byte_hex), "%02X ", buf[i]);
            strcat(hex, byte_hex);
        }
        nio_vram_grid_puts(36, y, 0, 0, hex, NIO_COLOR_WHITE, NIO_COLOR_GREEN);
        
        // ASCII column
        char ascii[BYTES_PER_LINE + 1];
        for (int i = 0; i < bytes_read; i++) {
            ascii[i] = (buf[i] >= 32 && buf[i] <= 126) ? buf[i] : '.';
        }
        ascii[bytes_read] = '\0';
        nio_vram_grid_puts(240, y, 0, 0, ascii, NIO_COLOR_WHITE, NIO_COLOR_CYAN);
    }
    
    // Footer
    nio_vram_fill(0, 230, 320, 10, NIO_COLOR_GRAY);
    nio_vram_grid_puts(0, 231, 0, 0, "Up/Down:Scroll  Esc:Exit", NIO_COLOR_GRAY, NIO_COLOR_WHITE);
    
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
            if (offset + page_size < file_size) {
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
            offset += page_size;
            if (offset >= file_size) {
                offset = file_size - page_size;
                if (offset < 0) offset = 0;
            }
        }
    }
    fclose(f);
}
