/*
 * Image viewer
 * 
 * Uses stb_image for decoding and direct VRAM access for rendering.
 * NIO's nio_vram_pixel_set uses a 256-color palette, not raw RGB565,
 * so we bypass it and use lcd_blit directly for true-color display.
 */


#include <nspireio/nspireio.h>
#include <libndls.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "stb_image.h"

#include "image_viewer.h"
#include "ui.h" // For ui_draw_modal
#include "input.h" // For input_get_key

/*
 * Use stbi_load_from_memory for simplicity/safety with Nspire caching quirks.
 * We read the file fully into a buffer first.
 */

// Security Limits
#define MAX_FILE_SIZE (10 * 1024 * 1024)  // 10 MB max file size
#define MAX_IMAGE_DIM 8192                // Max 8192x8192 pixels

void image_viewer_open(const char *path) {
    // 1. Read file to memory
    FILE *f = fopen(path, "rb");
    if (!f) {
        ui_draw_modal("Error: Could not open file.");
        wait_key_pressed();
        wait_no_key_pressed();
        return;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    // Security: Validate file size
    if (fsize <= 0) {
        fclose(f);
        ui_draw_modal("Error: Invalid file size.");
        wait_key_pressed();
        wait_no_key_pressed();
        return;
    }
    if (fsize > MAX_FILE_SIZE) {
        fclose(f);
        ui_draw_modal("Error: File too large (>10MB).");
        wait_key_pressed();
        wait_no_key_pressed();
        return;
    }

    unsigned char *buffer = (unsigned char*)malloc(fsize);
    if (!buffer) {
        fclose(f);
        ui_draw_modal("Error: Out of memory.");
        wait_key_pressed();
        wait_no_key_pressed();
        return;
    }
    size_t read_bytes = fread(buffer, 1, fsize, f);
    fclose(f);
    
    if (read_bytes != (size_t)fsize) {
        free(buffer);
        ui_draw_modal("Error: File read mismatch.");
        wait_key_pressed();
        wait_no_key_pressed();
        return;
    }

    // 2. Decode
    int w, h, n;
    unsigned char *data = stbi_load_from_memory(buffer, fsize, &w, &h, &n, 3); // RGB, not RGBA
    free(buffer);

    if (!data) {
        ui_draw_modal("Error: Failed to decode image.");
        wait_key_pressed();
        wait_no_key_pressed();
        return;
    }
    
    // Security: Validate decoded dimensions
    if (w <= 0 || h <= 0 || w > MAX_IMAGE_DIM || h > MAX_IMAGE_DIM) {
        stbi_image_free(data);
        ui_draw_modal("Error: Image dimensions invalid.");
        wait_key_pressed();
        wait_no_key_pressed();
        return;
    }

    // 3. Allocate screen buffer (320x240 @ 16bpp = 153600 bytes)
    int screen_w = 320;
    int screen_h = 240;
    uint16_t *vram = (uint16_t*)malloc(screen_w * screen_h * sizeof(uint16_t));
    if (!vram) {
        stbi_image_free(data);
        ui_draw_modal("Error: VRAM alloc failed.");
        wait_key_pressed();
        wait_no_key_pressed();
        return;
    }
    
    // Clear to black
    memset(vram, 0, screen_w * screen_h * sizeof(uint16_t));

    // 4. Calculate scaling (integer math)
    int draw_w = w;
    int draw_h = h;
    
    if (w > screen_w || h > screen_h) {
        int ratio_w = (screen_w * 1000) / w;
        int ratio_h = (screen_h * 1000) / h;
        int ratio = (ratio_w < ratio_h) ? ratio_w : ratio_h;
        
        draw_w = (w * ratio) / 1000;
        draw_h = (h * ratio) / 1000;
    }
    
    int start_x = (screen_w - draw_w) / 2;
    int start_y = (screen_h - draw_h) / 2;

    // 5. Render to VRAM buffer
    for (int y = 0; y < draw_h; y++) {
        for (int x = 0; x < draw_w; x++) {
            int src_x = (x * w) / draw_w;
            int src_y = (y * h) / draw_h;
            
            if (src_x >= w) src_x = w - 1;
            if (src_y >= h) src_y = h - 1;

            unsigned char *pixel = data + (src_y * w + src_x) * 3; // RGB
            
            // RGB565: RRRRR GGGGGG BBBBB
            uint16_t color = ((pixel[0] & 0xF8) << 8) | ((pixel[1] & 0xFC) << 3) | (pixel[2] >> 3);
            
            int screen_x = start_x + x;
            int screen_y = start_y + y;
            
            if (screen_x >= 0 && screen_x < screen_w && screen_y >= 0 && screen_y < screen_h) {
                vram[screen_y * screen_w + screen_x] = color;
            }
        }
    }
    
    stbi_image_free(data);

    // 6. Blit to LCD
    lcd_blit(vram, SCR_320x240_565);

    // 7. Wait for exit
    while (1) {
        int k = input_get_key();
        if (k == NIO_KEY_ESC || k == 'q' || k == NIO_KEY_ENTER || k == NIO_KEY_BACKSPACE) {
            break;
        }
    }
    free(vram);
    
    // Restore NIO display
    nio_fflush(nio_get_default());
}
