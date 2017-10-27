#ifndef RTC_H
#define RTC_H

//-----------------------------------------------------
//CMOS/RTC Types
//-----------------------------------------------------

typedef struct {
	uint32_t second;
	uint32_t minute;
	uint32_t hour;
	uint32_t dayOfMonth;
	uint32_t month;
	uint32_t year;
} rtc_date_t;

//-----------------------------------------------------
//CMOS/RTC Constants
//-----------------------------------------------------

#define BIT(n)				(0x01<<(n))

#define RTC_IRQ				8			//RTC IRQ
#define RTC_HOOK			3			//Hook ID used for RTC
#define RTC_ERROR			0xFFFFFFFF  //Value used for errors interacting with CMOS/RTC

//Basic IO Ports

#define CMOS_SEL_REG		0x70	//IO Port to select a register to read/write from CMOS
#define CMOS_DATA_PORT		0x71	//IO Port to read/write selected register from CMOS

//RTC Registers

#define RTC_SECOND			0x00	//RTC Seconds register
#define RTC_ALARM_SECOND	0x01	//RTC Alarm seconds register
#define RTC_MINUTE			0x02	//RTC Minutes register
#define RTC_ALARM_MINUTE	0x03	//RTC Alarm minutes register
#define RTC_HOUR			0x04	//RTC Hours register
#define RTC_ALARM_HOUR		0x05	//RTC Alarm hours register
#define RTC_WEEKDAY			0x06	//RTC Day of week register
#define RTC_DAYMONTH		0x07	//RTC Day of month register
#define RTC_MONTH			0x08	//RTC Month register
#define RTC_YEAR			0x09	//RTC Year register
#define RTC_CENTURY			0x32	//RTC Century register BCD (not guaranteed to exist)
#define RTC_STATUS_A		0x0A	//RTC Status register A
#define RTC_STATUS_B		0x0B	//RTC Status register B
#define RTC_STATUS_C		0x0C	//RTC Status register C
#define RTC_STATUS_D		0x0D	//RTC Status register D

//RTC Important Bits

#define NMI_DISABLE			BIT(7)	//Non Maskable Interrupt disable bit (sent to port 0x70)

//RTC Status Register A

#define UIP					BIT(7)	//Whether time is currently being updated
#define RS0					BIT(0)	//Square wave / Periodic interrupts frequency
#define RS1					BIT(1)	//Square wave / Periodic interrupts frequency
#define RS2					BIT(2)	//Square wave / Periodic interrupts frequency
#define RS3					BIT(3)	//Square wave / Periodic interrupts frequency

//RTC Status Register B

#define CLOCK_UPDATE		BIT(7)	//If 0, RTC updates time registers normally
#define PIE					BIT(6)	//RTC Periodic Interrupt Enable bit
#define AIE					BIT(5)	//RTC Alarm Interrupt Enable bit
#define UIE					BIT(4)	//RTC Update Interrupt Enable bit
#define SQWE				BIT(3)	//Square wave enable bit
#define BINDATE				BIT(2)	//If 1 - using binary date
#define C24HOUR				BIT(1)	//If 1 - using 24-hour clock
#define DST					BIT(0)	//Whether Daylight Saving Time is enabled or not

//RTC Status Register C

#define IRQF				BIT(7)	//Whether there was any RTC interrupt
#define PIF					BIT(6)	//Periodic Interrupt Flag
#define AIF					BIT(5)	//Alarm Interrupt Flag
#define UIF					BIT(4)	//Update Interrupt Flag

//RTC Status Register D

#define CMOS_BATT			BIT(7)	//If 0 - CMOS Battery dead

//Valid centuries

#define CURRCENT			0x20	//21st Century as of program date (update if needed)
#define C20TH				0x19	//20th Century
#define C21ST				0x20	//21st Century
#define C22ND				0x21	//22nd Century

//Valid months

#define JAN					0x01
#define FEB					0x02
#define MAR					0x03
#define APR					0x04
#define MAY					0x05
#define JUN					0x06
#define JUL					0x07
#define AUG					0x08
#define SET					0x09
#define OCT					0x10
#define NOV					0x11
#define DEC					0x12

//Important times in binary

#define BIN60				59
#define BIN24				23

//-----------------------------------------------------
//RTC Function definitions
//-----------------------------------------------------

//Subscribe RTC interrupts
//@return RTC hook ID upon success, -1 otherwise
int8_t rtc_subscribe();

//Unsubscribes RTC interrupts
//@return 0 upon success, -1 otherwise
int8_t rtc_unsubscribe();

//Stops all interrupts for one processor
void asm_cli();

//Restores all interrupts for one processor
void asm_sti();

//Reads data from CMOS/RTC register
//@param reg - register to read from
//@return data upon success, RTC_ERROR otherwise
uint32_t rtc_read_register(uint32_t reg);

//Writes data to CMOS/RTC register (disables NMI)
//@param reg - register to write to
//@param data - byte to write
//@return 0 upon success, RTC_ERROR otherwise
uint32_t rtc_write_register(uint32_t reg, uint32_t data);

//Enables NMI by clearing bit 7 of port 0x70
//@return 0 upon success, RTC_ERROR otherwise
uint32_t rtc_enableNMI();

//Display RTC config
//@return 0 upon success, -1 otherwise
int8_t rtc_display_config();

//Display RTC date and time
//@return 0 upon success, -1 otherwise
int8_t rtc_display_date();

//Resets CMOS/RTC Status Register B configuration and changes NMI state
//@param isNMI_enabled - TRUE if NMI is enabled, FALSE otherwise
void rtc_reset(uint8_t isNMI_enabled);

//Sets the CMOS/RTC date, doesn't change weekday nor century
//@param date - structure containing the new date
//@return 0 upon success, -1 otherwise
int8_t rtc_set_date(rtc_date_t date);

//Fills rtc_date_t struct with current date
//@param date - struct to fill
//@return 0 upon success, -1 otherwise
int8_t rtc_get_date(rtc_date_t* date);

//Converts hh:mm:ss to decimal
//@param date - struct to convert
void rtc_BCDtoDEC(rtc_date_t* date);

//Converts hh:mm:ss to BCD
//@param date - struct to convert
void rtc_DECtoBCD(rtc_date_t* date);

//Sets and alarm "seconds" after the current time
//@param seconds - seconds after the current time the alarm should be programmed to
//@return 0 upon success, -1 otherwise
int8_t rtc_setalarm_s(uint32_t seconds);

#endif //RTC_H
