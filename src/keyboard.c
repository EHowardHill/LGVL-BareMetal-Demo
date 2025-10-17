#include "keyboard.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Scancode to ASCII mapping (US keyboard, simplified) */
static const char scancode_to_ascii[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, /* Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 
    0, /* Right shift */
    '*',
    0, /* Alt */
    ' ', /* Space */
};

static key_event_t key_queue[KEY_QUEUE_SIZE];
static volatile int queue_head = 0;
static volatile int queue_tail = 0;
static int extended_scancode = 0;

static void enqueue_key(key_event_t key) {
    int next = (queue_head + 1) % KEY_QUEUE_SIZE;
    if (next != queue_tail) {
        key_queue[queue_head] = key;
        queue_head = next;
    }
}

void keyboard_init(void) {
    /* Simple polling-based keyboard - no interrupts */
    queue_head = 0;
    queue_tail = 0;
    extended_scancode = 0;
}

void keyboard_handler(void) {
    /* Poll keyboard in main loop instead of using interrupts */
    /* Check if data is available and drain the buffer completely */
    while (inb(KEYBOARD_STATUS_PORT) & 0x01) {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        
        /* Check for extended scancode prefix */
        if (scancode == 0xE0) {
            extended_scancode = 1;
            continue; // Use continue to proceed to the next iteration of the while loop
        }
        
        /* Ignore 0xE1 multi-byte sequences */
        if (scancode == 0xE1) {
            extended_scancode = 0;
            continue;
        }
        
        key_event_t key;
        key.scancode = scancode & 0x7F;
        key.pressed = !(scancode & 0x80);
        key.ascii = 0;
        
        /* Handle extended scancodes (arrow keys) */
        if (extended_scancode) {
            extended_scancode = 0;
            
            /* Only process key presses */
            if (!key.pressed) {
                continue;
            }
            
            /* Map extended scancodes to special keys */
            switch (key.scancode) {
                case 0x48: key.ascii = 1; break;  /* Up arrow */
                case 0x50: key.ascii = 2; break;  /* Down arrow */
                case 0x4B: key.ascii = 3; break;  /* Left arrow */
                case 0x4D: key.ascii = 4; break;  /* Right arrow */
                default: continue;
            }
        } else {
            /* Regular scancode - only process presses */
            if (!key.pressed) {
                continue;
            }
            
            if (key.scancode < 128) {
                key.ascii = scancode_to_ascii[key.scancode];
            }
        }
        
        /* Only enqueue valid keys */
        if (key.ascii != 0) {
            enqueue_key(key);
        }
    }
}

int keyboard_has_key(void) {
    return queue_head != queue_tail;
}

key_event_t keyboard_get_key(void) {
    key_event_t key = {0, 0, 0};
    
    if (queue_head != queue_tail) {
        key = key_queue[queue_tail];
        queue_tail = (queue_tail + 1) % KEY_QUEUE_SIZE;
    }
    
    return key;
}