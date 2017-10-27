#ifndef _LCOM_I8254_H
#define _LCOM_I8254_H

//Constants for programming the i8254 Timer

#define TIMER_FREQ       1193182			//Clock frequency for timer in PC and AT
#define TIMER_HOOK_BIT	 0					//Timer interrupt bitmask

#define ERROR			 0xFF				//Timer get config error since value returned is int but status is an unsigned char/long value,
											//8 bits wouldn't work because the status byte is 8 bits

#define COUNTER_MIN		 18					//Minimum frequency allowed by i8254, equals a 16bit uint max on the loaded div value
#define COUNTER_MAX		 596591				//Maximum frequency allowed by i8254, equals a div value of 2

#define BIT(n)			 (0x01<<(n))

#define TIMER0_IRQ	     0					//Timer 0 IRQ line

#define TIMER_OUT        BIT(7)     		//Output line of timer status byte
#define TIMER_SEL(n)	 (n << 6)   		//Control word for Timer n (range 0-2)
#define TIMER(n)		 (0x40 + n) 		//Timer n count register (range 0-2)

#define MAX_UINT16		 0xFFFF				//Maximum value of an unsigned 16bit integer

//I/O port addresses

#define TIMER_0			 0x40				// Timer 0 count register
#define TIMER_1			 0x41				// Timer 1 count register
#define TIMER_2			 0x42				// Timer 2 count register
#define TIMER_CTRL		 0x43				// Control register

#define SPEAKER_CTRL	 0x61				// Register for speaker control

//Timer control

//Timer selection: bits 7 and 6

#define TIMER_SEL0		 0x00				// Control Word for Timer 0
#define TIMER_SEL1		 BIT(6)				// Control Word for Timer 1
#define TIMER_SEL2		 BIT(7)				// Control Word for Timer 2
#define TIMER_RB_CMD	 (BIT(7)|BIT(6))  	// Read Back Command

//Register selection: bits 5 and 4

#define TIMER_LSB		 BIT(4)  			// Initialize Counter LSB only
#define TIMER_MSB		 BIT(5)  			// Initialize Counter MSB only
#define TIMER_LSB_MSB	 (TIMER_LSB | TIMER_MSB) // Initialize LSB first and MSB afterwards

//Operating mode: bits 3, 2 and 1

#define TIMER_SQR_WAVE	 (BIT(2)|BIT(1)) 	// Mode 3: square wave generator
#define TIMER_RATE_GEN	 BIT(2)          	// Mode 2: rate generator
#define TIMER_MODE_0	 0x00 				// Mode 0: interrupt on terminal count
#define TIMER_MODE_1	 BIT(1)				// Mode 1: hardware retriggerable one-shot
#define TIMER_MODE_4	 BIT(3)				// Mode 4: software triggered strobe
#define TIMER_MODE_5	 (BIT(3)|BIT(1))	// Mode 5: hardware triggered strobe (retriggerable)

//Counting mode: bit 0

#define TIMER_BCD		 0x01   			// Count in BCD */
#define TIMER_BIN		 0x00   			// Count in binary */

//Read-back Command Format

#define TIMER_RB_COUNT_  BIT(5)
#define TIMER_RB_STATUS_ BIT(4)
#define TIMER_RB_SEL(n)  BIT((n)+1)

#endif //_LCOM_I8254_H
