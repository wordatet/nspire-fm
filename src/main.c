#include <nspireio/nspireio.h>
#include <libndls.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "fs.h"
#include "ui.h"
#include "input.h"
#include "editor.h"
#include "viewer.h"

/*
 * Validates a filename for safe use in the filesystem.
 * Rejects: empty names, '/', '..', '.', names containing '/' or '\0',
 * and names with leading/trailing spaces.
 * Returns 1 if valid, 0 if invalid.
 */
static int is_valid_filename(const char *name) {
    if (!name || strlen(name) == 0) return 0;
    
    // Reject reserved names
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return 0;
    if (strcmp(name, "/") == 0) return 0;
    
    // Reject names containing path separator
    if (strchr(name, '/') != NULL) return 0;
    
    // Reject leading/trailing spaces
    if (name[0] == ' ' || name[strlen(name) - 1] == ' ') return 0;
    
    // Reject names that are only spaces
    int all_spaces = 1;
    for (size_t i = 0; i < strlen(name); i++) {
        if (name[i] != ' ') { all_spaces = 0; break; }
    }
    if (all_spaces) return 0;
    
    return 1;
}
int main(int argc, char **argv) {
    // 1. Initialize Console
    nio_console csl;
    if (!nio_init(&csl, NIO_MAX_COLS, NIO_MAX_ROWS, 0, 0, NIO_COLOR_BLACK, NIO_COLOR_WHITE, true))
        return 1;
    nio_set_default(&csl);
    
    // 2. Initial Path
    char current_path[1024] = "/documents";
    if (argc > 1) {
        strncpy(current_path, argv[1], sizeof(current_path) - 1);
        current_path[sizeof(current_path) - 1] = '\0';
    }
    
    // Debug Log (Serial)
    uart_printf("App Start\n");
    
    file_list_t file_list = {0}; // CRITICAL: Init to zero so fs_free doesn't crash on first run
    int selection = 0;
    int scroll_offset = 0;
    
    // Clipboard State
    char clipboard_path[1024] = "";
    int clipboard_mode = 0; // 0=None, 1=Copy, 2=Cut
    
    // Sort State
    int sort_mode = SORT_NAME;
    
    // Scan initial
    uart_printf("Scanning %s...\n", current_path);
    if (fs_scan(current_path, &file_list) != 0) {
        uart_printf("Scan Failed\n");
        // Fallback
        strcpy(current_path, "/");
        fs_scan(current_path, &file_list);
    }
    fs_sort(&file_list, sort_mode);
    uart_printf("Scan Done. Count: %d\n", file_list.count);
    
    // 3. Event Loop
    while (1) {
        uart_printf("Loop Start. Path: %s\n", current_path);

        // Render
        ui_draw_list(&file_list, selection, scroll_offset);
        
        // Input (Robust)
        int c = input_get_key();
        
        // Logic
        if (c == NIO_KEY_DOWN) {
            if (selection < file_list.count - 1) {
                selection++;
                // Scroll down if needed
                if (selection >= scroll_offset + 25) { // MAX_VISIBLE_ROWS
                    scroll_offset++;
                }
            }
        } else if (c == NIO_KEY_UP) {
            if (selection > 0) {
                selection--;
                // Scroll up if needed
                if (selection < scroll_offset) {
                    scroll_offset--;
                }
            }
        } else if (c == NIO_KEY_ENTER || c == NIO_KEY_RIGHT) {
            open_file:
            // Open selected item
            if (file_list.count == 0) continue; // Skip if empty
            
            file_entry_t *sel = &file_list.entries[selection];
            if (sel->is_dir) {
                // Handle ".." as Go Up
                if (strcmp(sel->name, "..") == 0) {
                    goto go_up;
                }
            
                // Enter Dir
                char new_path[1024];
                if (strcmp(current_path, "/") == 0)
                    snprintf(new_path, sizeof(new_path), "/%s", sel->name);
                else
                    snprintf(new_path, sizeof(new_path), "%s/%s", current_path, sel->name);
                
                fs_scan(new_path, &file_list);
                strcpy(current_path, new_path);
                selection = 0;
                scroll_offset = 0;
            } else {
                // Open/Launch File
                char full_path[1024];
                if (strcmp(current_path, "/") == 0)
                    snprintf(full_path, sizeof(full_path), "/%s", sel->name);
                else
                    snprintf(full_path, sizeof(full_path), "%s/%s", current_path, sel->name);
                
                // YOLO: Open in editor unless it's a known binary type
                const char *ext = strrchr(sel->name, '.');
                int is_binary = 0;
                if (ext && (strcasecmp(ext, ".tns") == 0 ||
                            strcasecmp(ext, ".tno") == 0 ||
                            strcasecmp(ext, ".tco") == 0 ||
                            strcasecmp(ext, ".tcc") == 0 ||
                            strcasecmp(ext, ".png") == 0 ||
                            strcasecmp(ext, ".jpg") == 0 ||
                            strcasecmp(ext, ".jpeg") == 0 ||
                            strcasecmp(ext, ".bmp") == 0 ||
                            strcasecmp(ext, ".zip") == 0)) {
                    is_binary = 1;
                }
                
                if (is_binary) {
                    nl_exec(full_path, 0, NULL);
                } else {
                    editor_open(full_path);
                }
            }
        } else if (c == NIO_KEY_ESC || c == NIO_KEY_LEFT) {
            go_up:
            // Go Up
            if (strcmp(current_path, "/") != 0) {
                 // Strip last segment
                 char *last_slash = strrchr(current_path, '/');
                 if (last_slash == current_path) {
                     // We are at /abc, go to /
                     strcpy(current_path, "/");
                 } else if (last_slash) {
                     *last_slash = '\0';
                 }
                 
                 fs_scan(current_path, &file_list);
                 selection = 0;
                 scroll_offset = 0;
            } else {
                // At root, do nothing (User requested: "why esc exits?")
                // break; 
            }
        } else if (c == 'q') {
            if (ui_get_confirmation("Do you want to exit?")) {
                goto exit_app;
            }
        } else if (c == NIO_KEY_MENU) { // Menu options
             const char *options[] = {
                 "Open",
                 "View Hex",
                 "Copy",
                 "Cut",
                 "Paste",
                 "Delete",
                 "Rename",
                 "Sort: Name/Size",
                 "New Directory",
                 "New File",
                 "Exit"
             };
             int opt_count = 11;
             int opt_sel = 0;
             
             // Menu Loop
             while(1) {
                 ui_draw_menu(options, opt_count, opt_sel);
                 int m = input_get_key();
                 
                 if (m == NIO_KEY_DOWN) {
                     opt_sel = (opt_sel + 1) % opt_count;
                 } else if (m == NIO_KEY_UP) {
                     opt_sel = (opt_sel - 1 + opt_count) % opt_count;
                 } else if (m == NIO_KEY_ESC || m == NIO_KEY_LEFT || m == NIO_KEY_MENU) {
                     break; // Close menu
                 } else if (m == NIO_KEY_ENTER) {
                     // Execute Action
                     if (opt_sel == 0) { // Open
                         goto open_file;
                     } else if (opt_sel == 1) { // View Hex
                         if (file_list.count > 0 && strcmp(file_list.entries[selection].name, "..") != 0 && !file_list.entries[selection].is_dir) {
                             char full_path[1024];
                             if (strcmp(current_path, "/") == 0)
                                 snprintf(full_path, sizeof(full_path), "/%s", file_list.entries[selection].name);
                             else
                                 snprintf(full_path, sizeof(full_path), "%s/%s", current_path, file_list.entries[selection].name);
                             viewer_open(full_path);
                         }
                         break;
                     } else if (opt_sel == 2) { // Copy
                         if (file_list.count > 0 && strcmp(file_list.entries[selection].name, "..") != 0) {
                             if (strcmp(current_path, "/") == 0)
                                 snprintf(clipboard_path, sizeof(clipboard_path), "/%s", file_list.entries[selection].name);
                             else
                                 snprintf(clipboard_path, sizeof(clipboard_path), "%s/%s", current_path, file_list.entries[selection].name);
                             clipboard_mode = 1; // Copy
                             ui_draw_modal("Copied to clipboard");
                             wait_key_pressed();
                             wait_no_key_pressed();
                         }
                         break;
                     } else if (opt_sel == 3) { // Cut
                         if (file_list.count > 0 && strcmp(file_list.entries[selection].name, "..") != 0) {
                             if (strcmp(current_path, "/") == 0)
                                 snprintf(clipboard_path, sizeof(clipboard_path), "/%s", file_list.entries[selection].name);
                             else
                                 snprintf(clipboard_path, sizeof(clipboard_path), "%s/%s", current_path, file_list.entries[selection].name);
                             clipboard_mode = 2; // Cut
                             ui_draw_modal("Marked for move");
                             wait_key_pressed();
                             wait_no_key_pressed();
                         }
                         break;
                     } else if (opt_sel == 4) { // Paste
                         if (clipboard_mode > 0 && strlen(clipboard_path) > 0) {
                             // Extract filename from clipboard_path
                             const char *src_name = strrchr(clipboard_path, '/');
                             if (src_name) src_name++; else src_name = clipboard_path;
                             
                             char dst_path[1024];
                             if (strcmp(current_path, "/") == 0)
                                 snprintf(dst_path, sizeof(dst_path), "/%s", src_name);
                             else
                                 snprintf(dst_path, sizeof(dst_path), "%s/%s", current_path, src_name);
                             
                             int res = -1;
                             if (clipboard_mode == 1) { // Copy
                                 // Handle same-directory copy: generate unique name
                                 if (strcmp(clipboard_path, dst_path) == 0) {
                                     if (fs_generate_copy_name(clipboard_path, dst_path, sizeof(dst_path)) != 0) {
                                         ui_draw_modal("Too many copies");
                                         wait_key_pressed();
                                         wait_no_key_pressed();
                                         break;
                                     }
                                 }
                                 res = fs_copy_file(clipboard_path, dst_path);
                             } else if (clipboard_mode == 2) { // Cut (Move)
                                 // Same-path move is a no-op
                                 if (strcmp(clipboard_path, dst_path) == 0) {
                                     ui_draw_modal("Already here");
                                     wait_key_pressed();
                                     wait_no_key_pressed();
                                     break;
                                 }
                                 res = rename(clipboard_path, dst_path);
                                 if (res == 0) {
                                     clipboard_path[0] = '\0';
                                     clipboard_mode = 0;
                                 }
                             }
                             
                             if (res != 0) {
                                 ui_draw_modal("Paste failed");
                                 wait_key_pressed();
                                 wait_no_key_pressed();
                             } else {
                                 fs_scan(current_path, &file_list);
                                 fs_sort(&file_list, sort_mode);
                             }
                         } else {
                             ui_draw_modal("Clipboard is empty");
                             wait_key_pressed();
                             wait_no_key_pressed();
                         }
                         break;
                     } else if (opt_sel == 5) { // Delete
                         if (strcmp(file_list.entries[selection].name, "..") == 0) {
                             ui_draw_modal("Cannot delete '..'");
                             wait_key_pressed();
                             wait_no_key_pressed();
                         } else {
                            // Confirmation
                            char msg[270];
                            snprintf(msg, sizeof(msg), "Are you sure you want to delete %s?", file_list.entries[selection].name);
                            
                            if (ui_get_confirmation(msg)) {
                                // Delete logic
                                char full_path[1024];
                                if (strcmp(current_path, "/") == 0) 
                                    snprintf(full_path, sizeof(full_path), "/%s", file_list.entries[selection].name);
                                else
                                    snprintf(full_path, sizeof(full_path), "%s/%s", current_path, file_list.entries[selection].name);
                                
                                // Try remove (files) or rmdir (folders)
                                int res = remove(full_path); 
                                // If it's a dir and remove/rmdir failed (likely not empty), try recursive
                                if (res != 0 && file_list.entries[selection].is_dir) {
                                    res = fs_delete_recursive(full_path);
                                }
                                
                                if (res != 0) {
                                     ui_draw_modal("Directory not empty");
                                     wait_key_pressed();
                                     wait_no_key_pressed();
                                }
                                
                                fs_scan(current_path, &file_list);
                                fs_sort(&file_list, sort_mode);
                                if (selection >= file_list.count && selection > 0) selection--;
                            }
                         }
                         break;
                     } else if (opt_sel == 6) { // Rename
                         if (strcmp(file_list.entries[selection].name, "..") == 0) {
                             ui_draw_modal("Cannot rename '..'");
                             wait_key_pressed();
                             wait_no_key_pressed();
                         } else {
                             char new_name[256];
                             strncpy(new_name, file_list.entries[selection].name, sizeof(new_name));
                             
                             if (ui_get_string("Rename to:", new_name, sizeof(new_name))) {
                                 // Validate new name
                                 if (!is_valid_filename(new_name)) {
                                     ui_draw_modal("Bad name");
                                     wait_key_pressed();
                                     wait_no_key_pressed();
                                     break;
                                 }
                                 
                                 // Perform Rename
                                 char old_full[1024];
                                 char new_full[1024];
                                 
                                 if (strcmp(current_path, "/") == 0) {
                                     snprintf(old_full, sizeof(old_full), "/%s", file_list.entries[selection].name);
                                     snprintf(new_full, sizeof(new_full), "/%s", new_name);
                                 } else {
                                     snprintf(old_full, sizeof(old_full), "%s/%s", current_path, file_list.entries[selection].name);
                                     snprintf(new_full, sizeof(new_full), "%s/%s", current_path, new_name);
                                 }
                                 
                                 // Check if destination exists
                                 struct stat st;
                                 if (stat(new_full, &st) == 0) {
                                     ui_draw_modal("File or directory already exists");
                                     wait_key_pressed();
                                     wait_no_key_pressed();
                                 } else {
                                     if (rename(old_full, new_full) != 0) {
                                         ui_draw_modal("Rename failed");
                                         wait_key_pressed();
                                         wait_no_key_pressed();
                                     }
                                 }
                                 
                                 fs_scan(current_path, &file_list);
                                 fs_sort(&file_list, sort_mode);
                             }
                         }
                         break;
                     } else if (opt_sel == 7) { // Sort
                         sort_mode = (sort_mode == SORT_NAME) ? SORT_SIZE : SORT_NAME;
                         fs_sort(&file_list, sort_mode);
                         break;
                     } else if (opt_sel == 8) { // New Folder
                         char name[64] = "";
                         if (ui_get_string("Folder Name:", name, sizeof(name))) {
                             if (!is_valid_filename(name)) {
                                 ui_draw_modal("This name is not allowed");
                                 wait_key_pressed();
                                 wait_no_key_pressed();
                             } else {
                                 char new_dir[1024];
                                 if (strcmp(current_path, "/") == 0)
                                     snprintf(new_dir, sizeof(new_dir), "/%s", name);
                                 else
                                     snprintf(new_dir, sizeof(new_dir), "%s/%s", current_path, name);
                                 
                                 // Create Directory
                                 if (mkdir(new_dir, 0755) != 0) {
                                     ui_draw_modal("Name in use");
                                     wait_key_pressed();
                                     wait_no_key_pressed();
                                 }
                                 
                                 // Refresh
                                 fs_scan(current_path, &file_list);
                                 fs_sort(&file_list, sort_mode);
                             }
                         }
                         break;
                         break;
                     } else if (opt_sel == 9) { // New File
                         char name[64] = "";
                         if (ui_get_string("File Name:", name, sizeof(name))) {
                             // First validate the filename for dangerous characters
                             if (!is_valid_filename(name)) {
                                 ui_draw_modal("This name is not allowed");
                                 wait_key_pressed();
                                 wait_no_key_pressed();
                                 break;
                             }
                             
                             // Validate extension (prevent OS panic on unknown types)
                             const char *valid_ext[] = {
                                 ".tns", ".txt", ".zip", ".bmp", ".png", ".jpg", ".jpeg",
                                 ".py", ".lua", ".xml", ".csv", NULL
                             };
                             int ext_ok = 0;
                             const char *dot = strrchr(name, '.');
                             if (dot == NULL) {
                                 ext_ok = 1; // No extension is OK
                             } else {
                                 for (int i = 0; valid_ext[i]; i++) {
                                     if (strcasecmp(dot, valid_ext[i]) == 0) {
                                         ext_ok = 1;
                                         break;
                                     }
                                 }
                             }
                             
                             if (!ext_ok) {
                                 ui_draw_modal("Extension not allowed");
                                 wait_key_pressed();
                                 wait_no_key_pressed();
                                 break;
                             }
                             
                             char new_file[1024];
                             if (strcmp(current_path, "/") == 0)
                                 snprintf(new_file, sizeof(new_file), "/%s", name);
                             else
                                 snprintf(new_file, sizeof(new_file), "%s/%s", current_path, name);
                             
                             // Check if exists
                             FILE *f = fopen(new_file, "r");
                             if (f) {
                                 fclose(f);
                                 ui_draw_modal("Name in use");
                                 wait_key_pressed();
                                 wait_no_key_pressed();
                             } else {
                                 // Create empty file
                                 f = fopen(new_file, "w");
                                 if (f) fclose(f);
                             }
                             
                             // Refresh
                             fs_scan(current_path, &file_list);
                             fs_sort(&file_list, sort_mode);
                         }
                         break;
                     } else if (opt_sel == 10) { // Exit
                         goto exit_app;
                     }
                     break; 
                 }
                 
                 // Menu overlay uses double buffer, no need to redraw background
             }
             // Force redraw of main list upon exit
             ui_draw_list(&file_list, selection, scroll_offset);
        }
    }
    
    exit_app:
    nio_free(&csl);
    return 0;
}
