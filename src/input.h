#ifndef INPUT_H
#define INPUT_H

#define NIO_KEY_ENTER 0x0A // \n
#define NIO_KEY_LEFT  0x83
#define NIO_KEY_RIGHT 0x84
#define NIO_KEY_MENU  0x85
#define NIO_KEY_BACKSPACE 0x08

int input_get_key(void);

#endif
