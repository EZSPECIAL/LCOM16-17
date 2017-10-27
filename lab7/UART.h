#ifndef UART_H
#define UART_H

//-----------------------------------------------------
//CMOS/RTC Constants
//-----------------------------------------------------

#define BIT(n)			(0x01<<(n))

#define COM1_IRQ 		4
#define COM1_HOOK 		4
#define COM2_IRQ		3
#define COM2_HOOK		5

#define UART_ERROR		0xFFFFFFFF	//Error value that doesn't interfere with regular UART operation
#define RCV_ERROR		0x00		//Error on reading COM after receive data interrupt
#define UART_CLOCK		115200

//Serial port address

#define COM1_BASE 		0x3F8
#define COM2_BASE		0x2F8

//UART register offsets

#define RBR 			0x00 //Receiver Buffer Register (if reading)
#define THR 			0x00 //Transmitter Holding Register (if writing)
#define IER 			0x01 //Interrupt Enable Register (read/write)
#define IIR 			0x02 //Interrupt Identification Register (if reading)
#define FCR 			0x02 //FIFO Control Register (if writing)
#define LCR 			0x03 //Line Control Register (read/write)
#define MCR 			0x04 //Modem Control Register (read/write)
#define LSR 			0x05 //Line Status Register (if reading)
#define MSR 			0x06 //Modem Status Register (if reading)
#define SPR 			0x07 //Scratch Register (read/write)

#define DLL 			0x00 //Divisor Latch LSB (if DLAB on LCR = 1)
#define DLM 			0x01 //Divisor Latch MSB (if DLAB on LCR = 1)

//Line Control Register

#define LCR_DEFAULT		0x03						//Default configuration of LCR on MINIX

#define WLENGTH 		(BIT(0) | BIT(1)) 		 	//Amount of data bits
#define STOPB 			BIT(2) 					 	//Number of stop bits (0 - 1 / 1 - 2)
#define PARITY 			(BIT(3) | BIT(4) | BIT(5))	//Type of parity
#define BREAK_EN 		BIT(6) 					 	//Enable break state, line stays logical 0
#define DLAB 			BIT(7) 					 	//Divisor Latch Access Bit

#define WL5B			0x00						//5 data bits
#define WL6B			BIT(0)						//6 data bits
#define WL7B			BIT(1)						//7 data bits
#define WL8B			(BIT(0) | BIT(1))			//8 data bits

#define STOP1			0x00						//1 stop bit
#define STOP2			BIT(2)						//2 stop bits

#define PAR_NONE		0x00						//No parity
#define PAR_ODD			BIT(3)						//Odd parity
#define PAR_EVEN		(BIT(3) | BIT(4))			//Even parity
#define PAR_MARK		(BIT(3) | BIT(5))			//Mark parity
#define PAR_SPACE		(BIT(3) | BIT(4) | BIT(5))	//Space parity

//Line Status Register

#define DATA_READY 		BIT(0) //Data available to read on RBR or FIFO
#define OVERRUN 		BIT(1) //Overrun error flag
#define SER_PAR 		BIT(2) //Parity error flag
#define FRAME_ERROR 	BIT(3) //Framing error flag
#define BREAK_INT 		BIT(4) //1 if long string of 0's detected
#define THRE 			BIT(5) //THR is empty
#define ALL_EMPTY 		BIT(6) //THR, transmit FIFO and shift register all empty
#define FIFO_ERROR 		BIT(7) //Any error occurred and data in FIFO is unreliable

//Interrupt Enable Register

#define IER_DEFAULT		0x0F	//Default configuration of IER on MINIX

#define RDI_EN 			BIT(0)	//Enable received data interrupt
#define TEI_EN 			BIT(1)	//Enable transmitter empty interrupt
#define RLS_EN 			BIT(2)	//Enable receiver line status interrupt

//Interrupt Identification Register

#define INT_STAT 		BIT(0) 					 	//0 - interrupt pending
#define INT_ORIG 		(BIT(1) | BIT(2) | BIT(3))	//Interrupt origin bits
#define FIFO64 			BIT(5) 				 	 	//64-byte FIFO
#define FIFO_STAT 		(BIT(6) | BIT(7)) 		 	//FIFO status bits

//FIFO Control Register

#define FCR_DEFAULT		0xC6				//Default configuration, disables and clears FIFOs
#define FCR_TEST		0xE7				//Value used to test FIFO capabilities of installed UART chip

#define TRIGGERLV		(BIT(6) | BIT(7))	//FIFO interrupt trigger level
#define EN_FIFO64		BIT(5)				//Enable 64-byte FIFO on some UART chips
#define CLR_TXFIFO		BIT(2)				//Clear transmit FIFO
#define CLR_RXFIFO		BIT(1)				//Clear receive FIFO
#define EN_FIFO			BIT(0)				//Enable FIFOs

//Scratch Register

#define SPR_TEST		0x2A				//Value to test Scratch Register existence

//-----------------------------------------------------
//CMOS/RTC Types
//-----------------------------------------------------

typedef struct {
	uint32_t dataB;
	uint32_t stopB;
	uint32_t parity;
	uint32_t bitrate;
	uint32_t interrupts;
} UART_config_t;

static UART_config_t receive8N1_9600 = {.dataB = WL8B, .stopB = STOP1, .parity = PAR_NONE, .bitrate = 9600, .interrupts = RDI_EN};
static UART_config_t transmit8N1_9600 = {.dataB = WL8B, .stopB = STOP1, .parity = PAR_NONE, .bitrate = 9600, .interrupts = 0x00};
static UART_config_t uart_minix = {.dataB = WL8B, .stopB = STOP1, .parity = PAR_NONE, .bitrate = 9600, .interrupts = RDI_EN | TEI_EN | RLS_EN};

//-----------------------------------------------------
//RTC Function definitions
//-----------------------------------------------------

//Subscribes COM1 interrupts
//@return 0 upon success, -1 otherwise
int8_t uart_subscribe();

//Unsubscribe COM1 interrupts
//@return 0 upon success, -1 otherwise
int8_t uart_unsubscribe();

//Reads data from UART port and returns it
//@param base_address - base address of COM port
//@param port - offset of COM port
//@return data upon success, UART_ERROR otherwise
uint32_t uart_read(uint32_t base_address, uint32_t port);

//Writes data to UART port
//@param base_address - base address of COM port
//@param port - offset of COM port
//@param data - data to write to COM port
//@return 0 upon success, UART_ERROR otherwise
uint32_t uart_write(uint32_t base_address, uint32_t port, uint32_t data);

//Resets current COM configuration to MINIX standard, sets LCR, IER, FCR and bitrate
//@param base_address - base address of COM port to reset
void uart_reset(uint32_t base_address);

//Initializes a struct with the configuration of the current COM
//@param base_address - base address of COM port
//@param config - pointer to struct to initialize
//@return 0 upon success, -1 otherwise
int8_t uart_get_config(uint32_t base_address, UART_config_t* config);

//Output UART version using known features as tests for the version
//@param base_address - base address of COM to test
//@return 0 upon success, -1 otherwise
int8_t uart_version(uint32_t base_address);

//Display COM configuration in readable output
//@param config - config to parse and display
//@return 0 upon success, -1 otherwise
int8_t uart_disp_conf(UART_config_t config);

//Set configuration of COM
//@param base_address - base address of COM port to configure
//@param config - struct with configurations to set COM to
//@return 0 upon success, -1 otherwise
int8_t uart_set_conf(uint32_t base_address, UART_config_t config);

//Sends a byte through COM1
//@param byte - byte to send
//@return 0 upon success, -1 otherwise
int8_t uart_send(uint8_t byte);

//Receives a byte from RBR, reads IIR then LSR to check errors
//@return data upon success, UART_ERROR if kernel calls failed, RCV_ERROR if LSR had error flags activated
uint32_t uart_receive();

#endif //UART_H
