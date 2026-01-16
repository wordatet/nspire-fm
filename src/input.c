/* input handling 
 * 
 * This module provides a robust input function that handles 
 * hardware keys and typed text. It helps to abstract the 
 * input handling from the rest of the code.
 */


#include <nspireio/nspireio.h>
#include <libndls.h>
#include "input.h" // For defines

// Robust input function
int input_get_key(void) {
    // 1. Wait for any hardware key press
    wait_key_pressed();
    
    // 2. Check for keys that nspireio might ignore or that we want to override
    // Priority: Menu/Ctrl -> Left/Right -> Enter
    
    // MENU / CTRL
    if (isKeyPressed(KEY_NSPIRE_MENU) || isKeyPressed(KEY_NSPIRE_CTRL)) {
        wait_no_key_pressed();
        return NIO_KEY_MENU;
    }
    
    // LEFT
    if (isKeyPressed(KEY_NSPIRE_LEFT)) {
        wait_no_key_pressed();
        return NIO_KEY_LEFT;
    }
    
    // RIGHT
    if (isKeyPressed(KEY_NSPIRE_RIGHT)) {
        wait_no_key_pressed();
        return NIO_KEY_RIGHT;
    }

    // ENTER (Explicit check, though nio handles it usually, but let's be safe)
    if (isKeyPressed(KEY_NSPIRE_ENTER)) {
        wait_no_key_pressed();
        return NIO_KEY_ENTER;
    }

    // 3. Fallback to nspireio for typed text / standard keys
    
    int c = nio_getch(nio_get_default());
    
    // Safety mapping for Enter if nio mapped it to \n
    if (c == '\n') c = NIO_KEY_ENTER;
    
    if (c != 0) {
        wait_no_key_pressed();
    }
    
    return c;
}
