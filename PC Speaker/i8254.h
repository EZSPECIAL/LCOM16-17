#ifndef _LCOM_I8254_H_
#define _LCOM_I8254_H_

/** @defgroup i8254 i8254
 * @{
 *
 * Constants for programming the i8254 Timer.
 */

#define TIMER_FREQ       1193182			/**< @brief clock frequency for timer in PC and AT */
#define TIMER_HOOK_BIT	 0					//Timer interrupt bitmask

#define ERROR			 0xFF				//Timer get config error since value returned is int but status is an unsigned char/long value, 8 bits wouldn't work because the status byte is 8 bits
#define COUNTER_MIN		 18					//Minimum frequency allowed by i8254, equals a 16bit uint max on the loaded div value
#define COUNTER_MAX		 596591				//Maximum frequency allowed by i8254, equals a div value of 2

#define BIT(n)			 (0x01<<(n))

#define TIMER0_IRQ	     0					/**< @brief Timer 0 IRQ line */

#define TIMER_OUT        BIT(7)     		//Output line of timer status byte
#define TIMER_SEL(n)	 (n << 6)   		//Control word for Timer n (range 0-2)
#define TIMER(n)		 (0x40 + n) 		//Timer n count register (range 0-2)

#define MAX_UINT16		 0xFFFF				//Maximum value of an unsigned 16bit integer

/* I/O port addresses */

#define TIMER_0			 0x40				/**< @brief Timer 0 count register */
#define TIMER_1			 0x41				/**< @brief Timer 1 count register */
#define TIMER_2			 0x42				/**< @brief Timer 2 count register */
#define TIMER_CTRL		 0x43				/**< @brief Control register */

#define SPEAKER_CTRL	 0x61				/**< @brief Register for speaker control  */

/* Timer control */

/* Timer selection: bits 7 and 6 */

#define TIMER_SEL0		 0x00				/**< @brief Control Word for Timer 0 */
#define TIMER_SEL1		 BIT(6)				/**< @brief Control Word for Timer 1 */
#define TIMER_SEL2		 BIT(7)				/**< @brief Control Word for Timer 2 */
#define TIMER_RB_CMD	 (BIT(7)|BIT(6))  	/**< @brief Read Back Command */

/* Register selection: bits 5 and 4 */

#define TIMER_LSB		 BIT(4)  			/**< @brief Initialize Counter LSB only */
#define TIMER_MSB		 BIT(5)  			/**< @brief Initialize Counter MSB only */
#define TIMER_LSB_MSB	 (TIMER_LSB | TIMER_MSB) /**< @brief Initialize LSB first and MSB afterwards */

/* Operating mode: bits 3, 2 and 1 */

#define TIMER_SQR_WAVE	 (BIT(2)|BIT(1)) 	/**< @brief Mode 3: square wave generator */
#define TIMER_RATE_GEN	 BIT(2)          	/**< @brief Mode 2: rate generator */
#define TIMER_MODE_0	 0x00 				/** Mode 0: interrupt on terminal count*/
#define TIMER_MODE_1	 BIT(1)				/** Mode 1: hardware retriggerable one-shot*/
#define TIMER_MODE_4	 BIT(3)				/** Mode 4: software triggered strobe*/
#define TIMER_MODE_5	 (BIT(3)|BIT(1))	/** Mode 5: hardware triggered strobe (retriggerable)*/

/* Counting mode: bit 0 */

#define TIMER_BCD		 0x01   			/**< @brief Count in BCD */
#define TIMER_BIN		 0x00   			/**< @brief Count in binary */

/* READ-BACK COMMAND FORMAT */

#define TIMER_RB_COUNT_  BIT(5)
#define TIMER_RB_STATUS_ BIT(4)
#define TIMER_RB_SEL(n)  BIT((n)+1)

//Piano notes and frequencies

#define A0  28
#define AS0 29
#define BB0 29
#define B0  31

#define C1  33
#define CS1 35
#define DB1 35
#define D1  37
#define DS1 39
#define EB1 39
#define E1  41
#define F1  44
#define FS1 46
#define GB1 46
#define G1  49
#define GS1 52
#define AB1 52
#define A1  55
#define AS1 58
#define BB1 58
#define B1  62

#define C2  65
#define CS2 69
#define DB2 69
#define D2  73
#define DS2 78
#define EB2 78
#define E2  82
#define F2  87
#define FS2 92
#define GB2 92
#define G2  98
#define GS2 104
#define AB2 104
#define A2  110
#define AS2 116
#define BB2 116
#define B2  123

#define C3  131
#define CS3 138
#define DB3 138
#define D3  147
#define DS3 156
#define EB3 156
#define E3  165
#define F3  175
#define FS3 185
#define GB3 185
#define G3  196
#define GS3 208
#define AB3 208
#define A3  220
#define AS3 233
#define BB3 233
#define B3  247

#define C4  262
#define CS4 277
#define DB4 277
#define D4  294
#define DS4 311
#define EB4 311
#define E4  330
#define F4  349
#define FS4 370
#define GB4 370
#define G4  392
#define GS4 415
#define AB4 415
#define A4  440
#define AS4 466
#define BB4 466
#define B4  494

#define C5  523
#define CS5 554
#define DB5 554
#define D5  587
#define DS5 622
#define EB5 622
#define E5  659
#define F5  698
#define FS5 740
#define GB5 740
#define G5  784
#define GS5 831
#define AB5 831
#define A5  880
#define AS5 932
#define BB5 932
#define B5  988

#define C6  1046
#define CS6 1109
#define DB6 1109
#define D6  1175
#define DS6 1244
#define EB6 1244
#define E6  1318
#define F6  1397
#define FS6 1480
#define GB6 1480
#define G6  1568
#define GS6 1661
#define AB6 1661
#define A6  1760
#define AS6 1865
#define BB6 1865
#define B6  1976

#define C7  2093
#define CS7 2217
#define DB7 2217
#define D7  2349
#define DS7 2489
#define EB7 2489
#define E7  2637
#define F7  2794
#define FS7 2960
#define GB7 2960
#define G7  3136
#define GS7 3322
#define AB7 3322
#define A7  3520
#define AS7 3729
#define BB7 3729
#define B7  3951

#define C8  4186

//Music file states

#define NOTES		1
#define DURATION	2
#define NOTES_S		3
#define LOOP		4

/**@}*/

#endif /* _LCOM_I8254_H */
