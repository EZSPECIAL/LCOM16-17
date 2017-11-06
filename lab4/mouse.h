#ifndef MOUSE_H
#define MOUSE_H

typedef enum {INIT, DRAW, GOAL} state_t;
typedef enum {RDOWN, RUP, GESTURE} ev_type_t;

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

//Interrupt handler for mouse interrupts
//cnt - packet count limit
//Returns 0 on success, -1 otherwise
int mouse_int_handler(int mouse_irq, unsigned short cnt);

//Interrupt handler for timer/mouse interrupts, stops after "time" seconds
//Returns 0 on success, -1 otherwise
int mouse_timed_handler(int mouse_irq, int timer_irq, unsigned short time);

//Interrupt handler for mouse interrupts, exist upon gesture
//length - movement in Y direction needed to exit
//Returns 0 on success, -1 otherwise
int mouse_gesture_handler(int mouse_irq, short length);

//Prints mouse config
int config_print(unsigned char* config);

//Reads mouse config bytes and calls config_print
int mouse_config();

//State machine for test_gesture
void mouse_state(ev_type_t event);

//Sets up remote mode and reads packets by polling
int mouse_remote(unsigned long period, unsigned short cnt);

#endif //MOUSE_H
