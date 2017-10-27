#ifndef I8042_H
#define I8042_H

//Constants for programming the i8042 PS/2 Controller

//Useful macros
#define BIT(n) 				(0x01<<(n))
#define NELEMS(x)  			(sizeof(x) / sizeof((x)[0]))

//Interrupt constants
#define KBD_IRQ 			1 //Keyboard interrupt line
#define KBD_HOOK_BIT 		1 //Keyboard interrupt bitmask

//Useful constants
#define DELAY_US 			20000 	//20ms delay for KBD commands
#define TIMEOUT 			3 		//Number of times to retry a read/write operation
#define MAX_UINT16 			0xFFFF 	//Maximum value of an unsigned 16bit integer
#define KBD_ERROR 			0xFFFF 	//Error value that doesn't interfere with possible data values, response values or scancodes
#define ENABLED				0x01
#define DISABLED			0x00

//i8042 Ports
#define STATUS_PORT 		0x64 //Status register port (for reading)
#define KBD_OUT_BUF 		0x60 //Data port output
#define KBD_IN_BUF 			0x60 //Data port input
#define KBD_CMD_BUF 		0x64 //Command register port (for writing)
#define KBD_DATA_BUF 		0x60 //Data port

//Status register byte
#define OUT_BUF_STATUS 		BIT(0) //Status register output buffer status bit
#define IN_BUF_STATUS 		BIT(1) //Status register input buffer status bit
#define SYS_FLAG 			BIT(2) //Status register system flag bit
#define CMD_DATA 			BIT(3) //Status register command/data bit
#define TIME_OUT_ERROR 		BIT(6) //Status register time-out error bit
#define PARITY_ERROR 		BIT(7) //Status register parity error bit

//PS/2 Controller Commands (write to port 0x64)
#define FIRST_PS2_ENABLE 	0xAE //Enable first PS/2 port
#define FIRST_PS2_DISABLE 	0xAD //Disable first PS/2 port

//PS/2 Keyboard Commands (write to port 0x60)
#define KBD_ENABLE 			0xF4 //Enable scanning (keyboard will send scan codes) :: Returns 0xFA (ACK) or 0xFE (Resend)
#define KBD_DISABLE 		0xF5 //Disable scanning (may also restore default parameters) :: Returns 0xFA (ACK) or 0xFE (Resend)
#define KBD_LED 			0xED //Toggle keyboard LEDs :: Returns 0xFA (ACK) or 0xFE (Resend)

//PS/2 Keyboard Responses
#define KBD_ACK 			0xFA //Command acknowledged
#define KBD_RESEND 			0xFE //Resend last byte

//Scan code set 1 (make/break codes for scan code set 1)
#define ESC_BREAK	 		0x81
#define BREAK_1				0x82
#define BREAK_2				0x83
#define BREAK_3				0x84
#define TWOB_CODE 			0xE0 //Prefix of 2 Byte scancodes

#endif //I8042_H
