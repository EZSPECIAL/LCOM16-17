#ifndef _VIDEO_H
#define _VIDEO_H

//Macros

#define BIT(n)				(0x01<<(n))
#define PB2BASE(x) 			(((x) >> 4) & 0x0F000)	//Calculate segment base from a linear address
#define PB2OFF(x) 			((x) & 0x0FFFF)			//Calculate offset from a linear address

//Constants

#define TIMEOUT				3						//Timeout for general purposes

//Hardcoded values for vg_init() on VBE Mode 0x105

#define VRAM_PHYS_ADDR		0xE0000000 	//Virtual Box, Windows Host, MINIX 3.1.8 LCOM Guest
#define VRAM_PHYS_ADDR_T	0xDC000000 	//Toshiba Tecra 9000 with MINIX 3.1.8 LCOM
#define H_RES				1024
#define V_RES				768
#define BITS_PER_PIXEL		8

//VBE Constants

#define VBE_CONTROLLER_INFO	0x4F00		//VBE Function 00h, returns display controller information
#define VBE_MODE_INFO		0x4F01		//VBE Function 01h, returns information about specific VBE modes
#define SET_VBE_MODE		0x4F02		//VBE Function 02h, initializes display controller and sets a VBE mode
#define BIOS_VIDEO			0x10		//BIOS Video Services interrupt vector

#define LINEAR_FRAME		BIT(14)		//Bit that sets linear frame buffer for any mode
#define MODE105H			0x105
#define MODE_INFO_SIZE		256			//VBE 2.0 ModeInfoBlock struct has fixed 256 bytes
#define MAXRES				1600		//Max resolution a VBox supported VBE mode has, specifically 1600x1200
#define MAXRES_105			1024		//Max resolution for mode 0x105

#define FSUPPORTED			0x4F		//VBE returns this on AL if function is supported
#define FCALL_SUCCESS		0x00		//Function call succeeded
#define FCALL_FAIL			0x01		//Function call failed
#define FHARDWARE			0x02		//Function call is not supported in current hardware configuration
#define FINVALID			0x03		//Function call invalid in current video mode

//VBE Function 00h
#define VIDEOMODEPTR		0x00		//Uses physical address returned by function 00h to find available VBE modes
#define VBE_RESERVED		0x01		//Uses the reserved area in the vbe_controller_info_t struct to find available VBE modes
#define BOTH_MODES			0x02		//Uses both methods, useful for debugging or showcasing functionality

#define LINE_BREAK			12			//Number of video modes per line
#define RESERVED_SIZE		222			//Number of elements in Reserved area of vbe_controller_info_t struct

//VBE Function 07h
#define SET_DISPLAY_START	0x4F07		//VBE Function 07h, used for page flipping

//Palette Colors
#define MAX256				0xFF 		//Last color on a 256 color palette

#endif //_VIDEO_H
