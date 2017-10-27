#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/types.h>

#include "RTC.h"

static int rtc_hook = RTC_HOOK;

int8_t rtc_subscribe() {

	rtc_hook = RTC_HOOK;

	if(sys_irqsetpolicy(RTC_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &rtc_hook) != OK) {
		printf("RTC: subscribe: couldn't set policy\n");
		return -1;
	}

	if(sys_irqenable(&rtc_hook) != OK) {
		printf("RTC: subscribe: couldn't enable interrupts\n");
		return -1;
	}

	return RTC_HOOK;
}


int8_t rtc_unsubscribe() {
    if(sys_irqdisable(&rtc_hook) != OK) {
    	printf("RTC: unsubscribe: couldn't disable interrupts\n");
    	return -1;
    }

	if(sys_irqrmpolicy(&rtc_hook) != OK) {
		printf("RTC: unsubscribe: couldn't remove policy\n");
		return -1;
	}

	return 0;
}


void asm_cli() {
	asm("cli");
}


void asm_sti() {
	asm("sti");
}


uint32_t rtc_read_register(uint32_t reg) {

	uint32_t data;

	if(sys_outb(CMOS_SEL_REG, reg) != OK) {
		printf("RTC: read: output to CMOS Select Register failed\n");
		return RTC_ERROR;
	}

	if(sys_inb(CMOS_DATA_PORT, &data) != OK) {
		printf("RTC: read: input from CMOS Data Port failed\n");
		return RTC_ERROR;
	}

	return data;
}


uint32_t rtc_write_register(uint32_t reg, uint32_t data) {

	if(sys_outb(CMOS_SEL_REG, NMI_DISABLE | reg) != OK) {
		printf("RTC: write: output to CMOS Select Register failed\n");
		return RTC_ERROR;
	}

	if(sys_outb(CMOS_DATA_PORT, data) != OK) {
		printf("RTC: write: output to CMOS Data Port failed\n");
		return RTC_ERROR;
	}

	return 0;
}


uint32_t rtc_enableNMI() {
	uint32_t data;

	if(sys_outb(CMOS_SEL_REG, RTC_STATUS_D) != OK) {
		printf("RTC: NMI: failed to clear NMI bit\n");
		return RTC_ERROR;
	}

	if(sys_inb(CMOS_DATA_PORT, &data) != OK) {
		printf("RTC: NMI: failed to discard byte\n");
		return RTC_ERROR;
	}

	return 0;
}


int8_t rtc_display_config() {

	uint32_t regA;
	uint32_t regB;
	uint32_t regD;

	asm_cli();

	regA = rtc_read_register(RTC_STATUS_A);
	regB = rtc_read_register(RTC_STATUS_B);
	regD = rtc_read_register(RTC_STATUS_D);

	asm_sti();

	if(regA == RTC_ERROR || regB == RTC_ERROR || regD == RTC_ERROR) {
		return -1;
	}

	printf("RTC Status Register A:\n\n");

	if((regA & UIP) == UIP) {
		printf("Update in progress: yes\n");
	} else printf("Update in progress: no\n");

	printf("\n");
	printf("RTC Status Register B:\n\n");

	if((regB & CLOCK_UPDATE) == CLOCK_UPDATE) {
		printf("Time registers update not enabled\n");
	} else printf("RTC updating time registers normally\n");

	if((regB & PIE) == PIE) {
		printf("Periodic Interrupt enabled\n");
	} else printf("Periodic Interrupt disabled\n");

	if((regB & AIE) == AIE) {
		printf("Alarm Interrupt enabled\n");
	} else printf("Alarm Interrupt disabled\n");

	if((regB & UIE) == UIE) {
		printf("Update Interrupt enabled\n");
	} else printf("Update Interrupt disabled\n");

	if((regB & SQWE) == SQWE) {
		printf("Square wave enabled\n");
	} else printf("Square wave disabled\n");

	if((regB & BINDATE) == BINDATE) {
		printf("Time registers in binary\n");
	} else printf("Time registers in BCD\n");

	if((regB & C24HOUR) == C24HOUR) {
		printf("Using 24-hour clock\n");
	} else printf("Using 12-hour clock\n");

	if((regB & DST) == DST) {
		printf("DST: on\n");
	} else printf("DST: off\n");

	printf("\n");
	printf("RTC Status Register D:\n\n");

	if((regD & CMOS_BATT) == CMOS_BATT) {
		printf("CMOS/RTC Battery: alive\n");
	} else printf("CMOS/RTC Battery: dead\n");

	return 0;
}


int8_t rtc_display_date() {

	uint32_t regA;

	do {

		asm_cli;

		regA = rtc_read_register(RTC_STATUS_A);

		if(regA == RTC_ERROR) {
			asm_sti;
			return -1;
		}

		if((regA & UIP) == UIP) {
			asm_sti;
		}

	} while((regA & UIP) == UIP);

	uint32_t century;
	uint32_t year;
	uint32_t month;
	uint32_t dayOfMonth;
	uint32_t weekday;
	uint32_t hour;
	uint32_t minute;
	uint32_t second;

	second = rtc_read_register(RTC_SECOND);
	minute = rtc_read_register(RTC_MINUTE);
	hour = rtc_read_register(RTC_HOUR);
	weekday = rtc_read_register(RTC_WEEKDAY);
	dayOfMonth = rtc_read_register(RTC_DAYMONTH);
	month = rtc_read_register(RTC_MONTH);
	year = rtc_read_register(RTC_YEAR);
	century = rtc_read_register(RTC_CENTURY);

	asm_sti();

	if(century == RTC_ERROR || year == RTC_ERROR || month == RTC_ERROR || dayOfMonth == RTC_ERROR ||
			weekday == RTC_ERROR || hour == RTC_ERROR || minute == RTC_ERROR || second == RTC_ERROR) {
		return -1;
	}

	switch(month) {
	case JAN:
		printf("Jan");
		break;
	case FEB:
		printf("Feb");
		break;
	case MAR:
		printf("Mar");
		break;
	case APR:
		printf("Apr");
		break;
	case MAY:
		printf("May");
		break;
	case JUN:
		printf("Jun");
		break;
	case JUL:
		printf("Jul");
		break;
	case AUG:
		printf("Aug");
		break;
	case SET:
		printf("Set");
		break;
	case OCT:
		printf("Oct");
		break;
	case NOV:
		printf("Nov");
		break;
	case DEC:
		printf("Dec");
		break;
	default:
		printf("N/A");
		break;
	}

	printf(" %02X", dayOfMonth);
	printf(" %02X:%02X:%02X", hour, minute, second);

	if(century == C20TH || century == C21ST || century == C22ND) {
		printf(" %02X%02X\n", century, year);
	} else {
		printf(" %02X%02X\n", CURRCENT, year);
	}

#if defined(DEBUG) && DEBUG == 1
	printf("\nRaw value:\n");
	printf("C: 0x%02X", century);	printf(" Y: 0x%02X", year); printf(" Mo: 0x%02X", month); printf(" DMo: 0x%02X\n", dayOfMonth);
	printf("WD: 0x%02X", weekday); printf(" H: 0x%02X", hour); printf(" M: 0x%02X", minute); printf(" S: 0x%02X\n", second);
#endif

	return 0;
}


void rtc_reset(uint8_t isNMI_enabled) {

	rtc_write_register(RTC_STATUS_B, C24HOUR);
	rtc_read_register(RTC_STATUS_C);

	if(isNMI_enabled == FALSE) {
		rtc_enableNMI();
	}

	asm_sti();
}


int8_t rtc_set_date(rtc_date_t date) {

	asm_cli();

	uint32_t regB;
	regB = rtc_read_register(RTC_STATUS_B);

	if(regB == RTC_ERROR) {
		asm_sti();
		return -1;
	}

	if(rtc_write_register(RTC_STATUS_B, CLOCK_UPDATE | regB) != 0) {
		rtc_reset(FALSE);
		return -1;
	}

	if(rtc_write_register(RTC_SECOND, date.second) != 0) {
		rtc_reset(FALSE);
		return -1;
	}
	if(rtc_write_register(RTC_MINUTE, date.minute) != 0) {
		rtc_reset(FALSE);
		return -1;
	}
	if(rtc_write_register(RTC_HOUR, date.hour) != 0) {
		rtc_reset(FALSE);
		return -1;
	}
	if(rtc_write_register(RTC_DAYMONTH, date.dayOfMonth) != 0) {
		rtc_reset(FALSE);
		return -1;
	}
	if(rtc_write_register(RTC_MONTH, date.month) != 0) {
		rtc_reset(FALSE);
		return -1;
	}
	if(rtc_write_register(RTC_YEAR, date.year) != 0) {
		rtc_reset(FALSE);
		return -1;
	}

	if(rtc_write_register(RTC_STATUS_B, regB) != 0) {
		rtc_reset(FALSE);
		return -1;
	}

	if(rtc_enableNMI() != 0) {
		rtc_enableNMI();
	}

	asm_sti;

	return 0;
}


int8_t rtc_get_date(rtc_date_t* date) {

	uint32_t regA;

	do {

		asm_cli;

		regA = rtc_read_register(RTC_STATUS_A);

		if(regA == RTC_ERROR) {
			asm_sti;
			return -1;
		}

		if((regA & UIP) == UIP) {
			asm_sti;
		}

	} while((regA & UIP) == UIP);

	date->second = rtc_read_register(RTC_SECOND);
	date->minute = rtc_read_register(RTC_MINUTE);
	date->hour = rtc_read_register(RTC_HOUR);
	date->dayOfMonth = rtc_read_register(RTC_DAYMONTH);
	date->month = rtc_read_register(RTC_MONTH);
	date->year = rtc_read_register(RTC_YEAR);

	asm_sti();

	if(date->year == RTC_ERROR || date->month == RTC_ERROR || date->dayOfMonth == RTC_ERROR ||
			date->hour == RTC_ERROR || date->minute == RTC_ERROR || date->second == RTC_ERROR) {
		return -1;
	}

	return 0;
}


void rtc_BCDtoDEC(rtc_date_t* date) {

	date->second = ((date->second & 0xF0) >> 4) * 10 + (date->second & 0x0F);
	date->minute = ((date->minute & 0xF0) >> 4) * 10 + (date->minute & 0x0F);
	date->hour = ((date->hour & 0xF0) >> 4) * 10 + (date->hour & 0x0F);
}


void rtc_DECtoBCD(rtc_date_t* date) {

	date->second = ((date->second / 10) << 4) | (date->second % 10);
	date->minute = ((date->minute / 10) << 4) | (date->minute % 10);
	date->hour = ((date->hour / 10) << 4) | (date->hour % 10);
}


int8_t rtc_setalarm_s(uint32_t seconds) {

	rtc_date_t date;

	rtc_get_date(&date);

	rtc_BCDtoDEC(&date);

	if(seconds > 60) {
		printf("RTC: alarm_s: tried to set alarm to more than 60 seconds after current time\n");
		return -1;
	}

	uint32_t regA;

	do {

		asm_cli;

		regA = rtc_read_register(RTC_STATUS_A);

		if(regA == RTC_ERROR) {
			asm_sti;
			return -1;
		}

		if((regA & UIP) == UIP) {
			asm_sti;
		}

	} while((regA & UIP) == UIP);

	if(date.second + seconds > BIN60) {
		date.second = date.second + seconds - BIN60 - 1;
		date.minute++;
		if(date.minute > BIN60) {
			date.minute = 0;
			date.hour++;
			if(date.hour > BIN24) {
				date.hour = 0;
			}
		}
	} else date.second += seconds;

	rtc_DECtoBCD(&date);

	if(rtc_write_register(RTC_ALARM_SECOND, date.second) != 0) {
		rtc_reset(FALSE);
		return -1;
	}
	if(rtc_write_register(RTC_ALARM_MINUTE, date.minute) != 0) {
		rtc_reset(FALSE);
		return -1;
	}
	if(rtc_write_register(RTC_ALARM_HOUR, date.hour) != 0) {
		rtc_reset(FALSE);
		return -1;
	}

	uint32_t regB;
	regB = rtc_read_register(RTC_STATUS_B);

	if(regB == RTC_ERROR) {
		asm_sti();
		return -1;
	}

	if(rtc_write_register(RTC_STATUS_B, AIE | regB) != 0) {
		rtc_reset(FALSE);
		return -1;
	}

	if(rtc_enableNMI() != 0) {
		rtc_enableNMI();
	}

	asm_sti;

	return 0;
}
