#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEY_QUEUE_SIZE 32

typedef struct {
    uint8_t scancode;
    uint8_t pressed;  /* 1 = pressed, 0 = released */
    char ascii;
} key_event_t;

void keyboard_init(void);
void keyboard_handler(void);
int keyboard_has_key(void);
key_event_t keyboard_get_key(void);

#endif