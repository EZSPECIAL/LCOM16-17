#ifndef KEYBOARD_H
#define KEYBOARD_H

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

//Interrupt handler to check for ESC break code
int kbd_check_esc();

//Interrupt handler to choose test_controller behavior
int kbd_controller_choice();

#endif //KEYBOARD_H
