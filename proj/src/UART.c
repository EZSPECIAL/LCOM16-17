#include <minix/syslib.h>
#include <minix/drivers.h>
#include <minix/types.h>

#include "UART.h"

static int com1_hook = COM1_HOOK;
static uint32_t uart_valid_rates[] = {50, 110, 220, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 0};

int8_t uart_subscribe() {

	com1_hook = COM1_HOOK;

	if(sys_irqsetpolicy(COM1_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &com1_hook) != OK) {
		printf("UART: subscribe: couldn't set policy for COM1\n");
		return -1;
	}

	if(sys_irqenable(&com1_hook) != OK) {
		printf("UART: subscribe: couldn't enable interrupts for COM1\n");
		return -1;
	}

	return COM1_HOOK;
}


int8_t uart_unsubscribe() {
    if(sys_irqdisable(&com1_hook) != OK) {
    	printf("UART: unsubscribe: couldn't disable interrupts for COM1\n");
    	return -1;
    }

	if(sys_irqrmpolicy(&com1_hook) != OK) {
		printf("UART: unsubscribe: couldn't remove policy for COM1\n");
		return -1;
	}

	return 0;
}


uint32_t uart_read(uint32_t base_address, uint32_t port) {

	uint32_t data;

	if(sys_inb(base_address + port, &data) != OK) {
		printf("UART: read: couldn't read from UART port\n");
		return UART_ERROR;
	}

	return data;
}


uint32_t uart_write(uint32_t base_address, uint32_t port, uint32_t data) {

	if(sys_outb(base_address + port, data) != OK) {
		printf("UART: write: couldn't write to UART port\n");
		return UART_ERROR;
	}

	return 0;
}


void uart_reset(uint32_t base_address) {
	uart_set_conf(base_address, uart_minix);
	uart_write(base_address, FCR, FCR_DEFAULT);
}


int8_t uart_get_config(uint32_t base_address, UART_config_t* config) {

	uint32_t data, lsb, msb;

	data = uart_read(base_address, LCR);

	if(data == UART_ERROR) {
		return -1;
	}

	config->dataB = (data & 0xFF) & WLENGTH;
	config->stopB = (data & 0xFF) & STOPB;
	config->parity = (data & 0xFF) & PARITY;

	if(uart_write(base_address, LCR, data | DLAB) != 0) {
		uart_reset(base_address);
		return -1;
	}

	lsb = uart_read(base_address, DLL);
	msb = uart_read(base_address, DLM);

	if(lsb == UART_ERROR || msb == UART_ERROR) {
		uart_reset(base_address);
		return -1;
	}

	config->bitrate = UART_CLOCK / (((msb << 8) & 0xFF00) | (lsb & 0xFF));

	if(uart_write(base_address, LCR, data) != 0) {
		uart_reset(base_address);
		return -1;
	}

	data = uart_read(base_address, IER);

	if(data == UART_ERROR) {
		return -1;
	}

	config->interrupts = (data & 0xFF) & (RDI_EN | TEI_EN | RLS_EN);

	return 0;
}


int8_t uart_version(uint32_t base_address) {

	if(uart_write(base_address, FCR, FCR_TEST) != 0) {
		uart_reset(base_address);
		return -1;
	}

	uint32_t fcr_test;
	fcr_test = uart_read(base_address, IIR);

	if(fcr_test == UART_ERROR) {
		uart_reset(base_address);
		return -1;
	}

	//If bit 6 of the IIR is not set then there's no FIFO
	if(fcr_test & BIT(6)) {
		//If bit 7 is not set then UART has the FIFO bug and is 16550
		if(fcr_test & BIT(7)) {
			//Check for 64-byte FIFO
			if((fcr_test & FIFO64) == FIFO64) {
				printf("UART is 16750\n");
				uart_reset(base_address);
				return 0;
			} else {
				printf("UART is 16550A\n");
				uart_reset(base_address);
				return 0;
			}
		} else {
			printf("UART is 16550\n");
			uart_reset(base_address);
			return 0;
		}
	} else {

		if(uart_write(base_address, SPR, SPR_TEST) != 0) {
			uart_reset(base_address);
			return -1;
		}

		uint32_t scratch_test;

		scratch_test = uart_read(base_address, SPR);

		if(scratch_test == UART_ERROR) {
			uart_reset(base_address);
			return -1;
		}

		if(scratch_test == SPR_TEST) {
			printf("UART is 16450\n");
			uart_reset(base_address);
			return 0;
		} else {
			printf("UART is 8250\n");
			uart_reset(base_address);
			return 0;
		}
	}

	uart_reset(base_address);
	return 0;
}


int8_t uart_disp_conf(UART_config_t config) {

	if((config.dataB & WL8B) == WL8B) {
		printf("Data bits: 8");
	} else if((config.dataB & WL7B) == WL7B) {
		printf("Data bits: 7");
	} else if((config.dataB & WL6B) == WL6B) {
		printf("Data bits: 6");
	} else if((config.dataB & WL5B) == WL5B) {
		printf("Data bits: 5");
	} else printf("Data bits: N/A");

	if((config.parity & PAR_SPACE) == PAR_SPACE) {
		printf("\tParity: space");
	} else if((config.parity & PAR_MARK) == PAR_MARK) {
		printf("\tParity: mark");
	} else if((config.parity & PAR_EVEN) == PAR_EVEN) {
		printf("\tParity: even");
	} else if((config.parity & PAR_ODD) == PAR_ODD) {
		printf("\tParity: odd");
	} else if((config.parity & PAR_NONE) == PAR_NONE) {
		printf("\tParity: none");
	} else printf("\tParity: N/A");

	if((config.stopB & STOP2) == STOP2) {
		printf("\tStop bits: 2");
	} else if((config.stopB & STOP1) == STOP1) {
		printf("\tStop bits: 1");
	} else printf("\tStop bits: N/A");

	printf("\tBitrate: %d\n", config.bitrate);

	if((config.interrupts & RDI_EN) == RDI_EN) {
		printf("RX Interrupt: ON");
	} else printf("RX Interrupt: OFF");

	if((config.interrupts & TEI_EN) == TEI_EN) {
		printf("\tTX Interrupt: ON");
	} else printf("\tTX Interrupt: OFF");

	if((config.interrupts & RLS_EN) == RLS_EN) {
		printf("\tLine Status Interrupt: ON");
	} else printf("\tLine Status Interrupt: OFF");

	printf("\n");
	return 0;
}


int8_t uart_set_conf(uint32_t base_address, UART_config_t config) {

	if(config.bitrate == 0) {
		printf("UART: setconf: bitrate can't be 0\n");
		uart_reset(base_address);
		return -1;
	}

	//Validate bitrate from a pool of common bitrates
	size_t i = 0;
	uint8_t found = FALSE;
	while(TRUE) {
		if(uart_valid_rates[i] == 0) {
			found = FALSE;
			break;
		}

		if(uart_valid_rates[i] == config.bitrate) {
			found = TRUE;
			break;
		}

		i++;
	}

	if(found == FALSE) {
		printf("UART: setconf: bitrate can't be %d\n", config.bitrate);
		uart_reset(base_address);
		return -1;
	}

	//Set DLAB to change bitrate
	uint32_t config_LCR = config.dataB | config.stopB | config.parity;

	if(uart_write(base_address, LCR, config_LCR | DLAB) != 0) {
		uart_reset(base_address);
		return -1;
	}

	//Calculate divider from bitrate
	uint32_t divider = UART_CLOCK / config.bitrate;
	uint8_t lsb = divider & 0xFF;
	uint8_t msb = (divider & 0xFF00) >> 8;

	if(uart_write(base_address, DLL, lsb) != 0) {
		uart_reset(base_address);
		return -1;
	}

	if(uart_write(base_address, DLM, msb) != 0) {
		uart_reset(base_address);
		return -1;
	}

	//Disable DLAB and set desired communication parameters
	if(uart_write(base_address, LCR, config_LCR) != 0) {
		uart_reset(base_address);
		return -1;
	}

	//Set desired interrupts
	uint32_t config_IER = config.interrupts;

	if(uart_write(base_address, IER, config_IER) != 0) {
		uart_reset(base_address);
		return -1;
	}

	return 0;
}


int8_t uart_send(uint8_t byte) {

	uint32_t lsr_data;
	uint32_t timeout = UART_TIMEOUT;

	do {
		lsr_data = uart_read(COM1_BASE, LSR);
		//tickdelay(micros_to_ticks(1000));

		if(lsr_data == UART_ERROR) {
			uart_reset(COM1_BASE);
			return -1;
		}

		timeout--;
		if(timeout == 0) {
			break;
		}
	} while(!((lsr_data & THRE) == THRE));

	if(timeout != 0) {
		if(uart_write(COM1_BASE, THR, byte) != 0) {
			uart_reset(COM1_BASE);
			return -1;
		}
	}

	return 0;
}


uint32_t uart_receive() {

	uint32_t received;
	uint32_t lsr_status, iir_status;

	iir_status = uart_read(COM1_BASE, IIR);

	if(iir_status == UART_ERROR) {
		return UART_ERROR;
	}

	lsr_status = uart_read(COM1_BASE, LSR);

	if(lsr_status == UART_ERROR) {
		return UART_ERROR;
	}

	received = uart_read(COM1_BASE, RBR);

	if(received == UART_ERROR) {
		return UART_ERROR;
	}

	if((lsr_status & (OVERRUN | SER_PAR | FRAME_ERROR)) != 0) {
		return RCV_ERROR;
	} else {
		return received;
	}

	return received;
}
