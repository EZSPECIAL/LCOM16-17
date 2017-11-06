#ifndef KEYBOARD_H
#define KEYBOARD_H

extern int hook_id;

int kbd_asm_handler(void);

//Sleep 20ms*cycles for i8042 interfacing
//Uses tickdelay from MINIX
void delay(int cycles);

//Tries to return keyboard control to MINIX, scancode_status defines whether scancodes should be re-enabled
void kbd_reset(int scancode_status);

//Writes command to specified port
//Returns 0 on success, KBD_ERROR otherwise
int kbd_write(unsigned long command, int port);

//Reads command from PS/2 controller output port
//Returns data read on success, KBD_ERROR otherwise
int kbd_read();

//Reads command from PS/2 controller output port but discards it
int kbd_discard();

//Subscribes keyboard interrupts
//Returns hook_id bit position on success, -1 otherwise
int kbd_subscribe_int();

//Unsubscribe keyboard interrupts
//Returns 0 on success, -1 otherwise
int kbd_unsubscribe_int();

//Writes command to port, does so by calling kbd_write followed by kbd_read for response
//Verifies response from PS/2 controller, resends if needed or returns
//Returns 0 on success, -1 otherwise
int kbd_command(unsigned long command, int port);

//Prints scancode received from keyboard
//Always returns 0
int kbd_scancode_print(unsigned long scancode);

//Interrupt handler for keyboard interrupts
//ass - defines whether to use C or asm handler
//Returns 0 on success, -1 otherwise
int kbd_int_handler(int kbd_irq, unsigned short ass);

//Interrupt handler for timer/keyboard interrupts, stops after "time" seconds
//Returns 0 on success, -1 otherwise
int kbd_timed_handler(int kbd_irq, int timer_irq, unsigned short time);

//Reads and prints scancodes with polling instead of interrupts. Enables interrupts on exit.
//Returns 0 on success, -1 otherwise
int kbd_poll();

#endif //KEYBOARD_H
