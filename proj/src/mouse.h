#ifndef MOUSE_H
#define MOUSE_H

//Sleep 20ms*cycles for i8042 interfacing
//Uses tickdelay from MINIX
void delay(int cycles);

//Tries to return mouse control to MINIX
void mouse_reset(int data_report);

//Writes command to specified port
//Returns 0 on success, KBD_ERROR otherwise
int mouse_write(unsigned long command, int port);

//Reads command from PS/2 controller output port
//Returns data read on success, KBD_ERROR otherwise
int mouse_read();

//Reads command from PS/2 controller output port but discards it
int mouse_discard();

//Writes command to port, does so by calling mouse_write followed by mouse_read for response
//Verifies response from PS/2 controller, resends if needed or returns
//Returns 0 on success, -1 otherwise
int mouse_command(unsigned long command);

//Subscribes mouse interrupts
//Returns hook_id bit position on success, -1 otherwise
int mouse_subscribe_int();

//Unsubscribe mouse interrupts
//Returns 0 on success, -1 otherwise
int mouse_unsubscribe_int();

//Prints mouse packets in user friendly way
//Returns 0 on success, -1 otherwise
int mouse_print_packet(unsigned char* packet);

//Prints mouse config
int config_print(unsigned char* config);

//Reads mouse config bytes and calls config_print
int mouse_config();

int8_t mouse_magic_sequence();

#endif //MOUSE_H
