#include <minix/syslib.h>
#include <minix/drivers.h>
#include <machine/int86.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <math.h>

#include "vbe.h"
#include "video.h"
#include "lmlib.h"
#include "timer.h"
#include "keyboard.h"
#include "i8042.h"

//Variables whose scope is video_gr.c

static phys_bytes video_phys;	//Physical address for VRAM
								//vg_init takes "mode" and returns virtual address of
								//VRAM, since it came already declared, had to create
								//auxiliary function to return this value and set it
								//so vg_init can use it later

static char *video_mem;			//Virtual address for VRAM
static char *double_buffer;		//Buffer to use for double buffering
static unsigned h_res;			//Horizontal screen resolution in pixels
static unsigned v_res;			//Vertical screen resolution in pixels
static unsigned bits_per_pixel; //Number of bits per pixel to represent color in VRAM

//Returns to default Minix 3 text mode (0x03: 25 x 80, 16 colors)
int vg_exit() {
  struct reg86u reg86;

  reg86.u.b.intno = 0x10; /* BIOS video services */

  reg86.u.b.ah = 0x00;    /* Set Video Mode function */
  reg86.u.b.al = 0x03;    /* 80x25 text mode*/

  if( sys_int86(&reg86) != OK ) {
      printf("\tvg_exit(): sys_int86() failed \n");
      return 1;
  } else {
		return 0;
  }
}


//Initialize static variables with the info from VBE Function 01h
int vg_init_values(unsigned short mode) {

	//Struct to store info from calling VBE Function 01h
	vbe_mode_info_t info;

	//Call VBE function 01h
	if(vbe_get_mode_info(mode, &info) != 0) {
		return -1;
	}

	//Initialize values for the mode specified
	h_res = info.XResolution;
	v_res = info.YResolution;
	bits_per_pixel = info.BitsPerPixel;
	video_phys = info.PhysBasePtr;

	double_buffer = (char*) malloc(h_res * v_res * (bits_per_pixel / 8));

	return info.PhysBasePtr;
}


//Uses the VBE INT 0x10 interface to set the desired
//graphics mode, maps VRAM to the process' address space
//Doesn't initialize values since vg_init_values() does that
void *vg_init(unsigned short mode) {

	struct reg86u reg86;

	//Call VBE Function 02h to set display mode
	reg86.u.w.ax = SET_VBE_MODE;
	reg86.u.w.bx = LINEAR_FRAME | mode;
	reg86.u.b.intno = BIOS_VIDEO;

	if(sys_int86(&reg86) != OK) {
		printf("vga: vg_init: sys_int86() failed\n");
		return NULL;
	}

	//Check function support in AL register
	if(reg86.u.b.al != FSUPPORTED) {
		printf("vga: vg_init: mode is not supported\n");
		return NULL;
	}

	//Check function call status in AH register
	if(reg86.u.b.ah != FCALL_SUCCESS) {

		switch(reg86.u.b.ah) {
		case FCALL_FAIL:
			printf("vga: vg_init: function call failed\n");
			break;
		case FHARDWARE:
			printf("vga: vg_init: function call is not supported in current hardware configuration\n");
			break;
		case FINVALID:
			printf("vga: vg_init: function call invalid in current video mode\n");
			break;
		default:
			printf("vga: vg_init: unknown VBE function error\n");
			break;
		}

		return NULL;
	}

	int r;
	struct mem_range mr;

	int vram_size = h_res * v_res * (bits_per_pixel / 8);

	//Allow memory mapping
	mr.mr_base = video_phys;
	mr.mr_limit = mr.mr_base + vram_size;

	if( OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr)))
		panic("sys_privctl (ADD_MEM) failed: %d\n", r);

	//Map memory
	video_mem = vm_map_phys(SELF, (void*)mr.mr_base, vram_size);

	if(video_mem == MAP_FAILED)
		panic("vga: vg_init: couldn't map video memory\n");

	return video_mem; //Virtual address of VRAM
}


//Draws pixel at x, y with "color"
//Can draw in 8bpp indexed, 16bpp, 24bpp, 32bpp as long as its linear framebuffer
int draw_pixel(unsigned short x, unsigned short y, unsigned long color) {

	if(x < h_res && y < v_res) {
		char* vram_t = double_buffer;
		vram_t += (y * h_res + x) * (bits_per_pixel / 8);

		switch(bits_per_pixel) {
		//Mostly used for packed pixel modes with a palette of 256 colors
		case 8:
			*vram_t = color & 0x000000FF;
			break;
		//No RGB 5:5:5 since that's a weird one
		//RGB 5:6:5
		case 16:
			*vram_t = (color & 0x0000001F) | (color & 0x000000E0);
			*(vram_t + 1) = ((color & 0x00000700) | (color & 0x0000F800)) >> 8;
			break;
		//RGB 8:8:8
		case 24:
			*vram_t = color & 0x000000FF;
			*(vram_t + 1) = (color & 0x0000FF00) >> 8;
			*(vram_t + 2) = (color & 0x00FF0000) >> 16;
			break;
		//ARGB 8:8:8:8
		case 32:
			*vram_t = color & 0x000000FF;
			*(vram_t + 1) = (color & 0x0000FF00) >> 8;
			*(vram_t + 2) = (color & 0x00FF0000) >> 16;
			*(vram_t + 3) = (color & 0xFF000000) >> 24;
		}

		return 0;
	} else return -1;
}


//Draw square at x, y coords with size and color passed as arguments
int draw_square(unsigned short x, unsigned short y, unsigned short size, unsigned long color) {

	int i, j;
	for(i = 0; i < size; i++) {
		for(j = 0; j < size; j++) {
			draw_pixel(x + j, y + i, color);
		}
	}

	memcpy(video_mem, double_buffer, h_res * v_res * (bits_per_pixel / 8));
	return 0;
}


//Draws line with an adaptation of Bresenham's Line Algorithm
//Source: https://en.wikipedia.org/wiki/Bresenham's_line_algorithm
//Algorithm adapted: http://www.roguebasin.com/index.php?title=Bresenham%27s_Line_Algorithm
int draw_line(unsigned short xi, unsigned short yi, unsigned short xf, unsigned short yf, unsigned long color) {

	int dx, dy; //Difference between start X/Y and endpoint X/Y
	int ix, iy; //Increment value of y and x
	int err;	//Accumulated error

	//Calculate deltas
	dx = xf - xi;
	dy = yf - yi;

	//Define increment based on deltas
	if(dx > 0)
		ix = 1;
	else ix = -1;
	if(dy > 0)
		iy = 1;
	else iy = -1;

	//Absolute and multiply by 2 to use (err = 2*dy - dx),
	//derived Bresenham's algorithm, from (err = abs(dy / dx))
	dx = abs(dx) << 1;
	dy = abs(dy) << 1;

	draw_pixel(xi, yi, color);

	//Iterate over X
    if (dx >= dy)
    {
        //(err = 2*dy - dx), dy is already 2*dy, dx is 2*dx so we divide
        err = (dy - (dx >> 1));

        while (xi != xf)
        {
            if ((err >= 0) && (err || (ix > 0)))
            {
                err -= dx;
                yi += iy;
            }

            err += dy;
            xi += ix;

            draw_pixel(xi, yi, color);
        }
    }

    //Iterate over Y
    else
    {
    	//(err = 2*dx - dy), dx is already 2*dx, dy is 2*dy so we divide
        err = (dx - (dy >> 1));

        while (yi != yf)
        {
            if ((err >= 0) && (err || (iy > 0)))
            {
                err -= dy;
                xi += ix;
            }

            err += dx;
            yi += iy;

            draw_pixel(xi, yi, color);
        }
    }

	memcpy(video_mem, double_buffer, h_res * v_res * (bits_per_pixel / 8));
    return 0;
}


//Draw sprite at xi, yi coords with width, height and sprite[] being the return from read_xpm
int draw_sprite(unsigned short xi, unsigned short yi, unsigned short width, unsigned short heigth, char *sprite) {

	int i, j;
	for (i = 0; i < heigth; i++) {
		for (j = 0; j < width; j++) {
			if(sprite[j+i*width] != 0)
				draw_pixel(xi+j, yi+i, sprite[j+i*width]);
		}
	}

	memcpy(video_mem, double_buffer, h_res * v_res * (bits_per_pixel / 8));
	return 0;
}

//Erase sprite at xi, yi coords with width, height and sprite[] being the return from read_xpm
int erase_sprite(unsigned short xi, unsigned short yi, unsigned short width, unsigned short heigth, char* sprite) {

	int i, j;
	for (i = 0; i < heigth; i++) {
		for (j = 0; j < width; j++) {
			if(sprite[j+i*width] != 0)
				draw_pixel(xi+j, yi+i, BLACK);
		}
	}

	memcpy(video_mem, double_buffer, h_res * v_res * (bits_per_pixel / 8));
	return 0;
}


//Returns a 20bit physical address when given a 32bit linear address base:offset
int vg_aux_offset(phys_bytes phys_address) {
	int offset = phys_address & 0xFFFF;
	int segment = ((phys_address >> 16) & 0xFFFF) << 4;

	return offset + segment;
}


//Calls VBE function 00h to fill vbe_controller_info_t struct
//isVBE2 decides whether VBE Signature should be preset to VBE2
//videoPtrMode has 3 modes
//		- VIDEOMODEPTR - uses the VideoModePtr to list VBE modes
//		- VBE_RESERVED - uses the Reserved area of the struct to list VBE modes
//		- BOTH_MODES   - uses both modes
int vg_controller(int isVBE2, int videoPtrMode) {

	//Memory map information of allocated block in 1st MiB
	mmap_t lmblock;

	//Initialize low memory and map it, roughly 1st MiB
	char* lmbase = lm_init();

	if(lmbase == NULL) {
		printf("vga: vg_controller: failed to map low memory\n");
		return -1;
	}

	//Allocate a memory block within that 1st MiB
	//sizeof(vbe_controller_info_t) is 512 bytes as specified by VBE 2.0
	char* lmalloc_v = lm_alloc(sizeof(vbe_controller_info_t), &lmblock);

	if(lmalloc_v == NULL) {
		printf("vga: vg_controller: failed to allocate block in low memory\n");
		return -1;
	}

	//Struct that will hold all the info returned by function 00h
	//Set its address to the virtual address of the allocated block in low memory
	vbe_controller_info_t* info_p = lmblock.virtual;

	//Preset VBE Signature to VBE2 if isVBE2 isn't 0
	if(isVBE2) {
		info_p->VbeSignature[0] = 'V';
		info_p->VbeSignature[1] = 'B';
		info_p->VbeSignature[2] = 'E';
		info_p->VbeSignature[3] = '2';
	}

	//Call interrupt 10h with function 00h of VBE
	if (vbe_get_controller_info(&lmblock, info_p) != 0) {
		return -1;
	}

	//Start of VBE 1.0 / 1.2 Info

	//VBE Signature, always VESA after calling function 00h
	int i;
	printf("VBE Signature: ");
	for(i = 0; i < 4; i++) {
		printf("%c", info_p->VbeSignature[i]);
	}
	printf("\n");

	//VBE Version, BCD where MSB is Major Version and LSB is Minor Version
	printf("VBE Version: %d.", ((info_p->VbeVersion & 0xFF00) >> 8));
	printf("%d\n", info_p->VbeVersion & 0x00FF);

	//lmbase is the virtual address of physical address 0x00000000
	//sum to lmbase the physical address stored in the struct, but only
	//after converting it to a 20bit physical address
	char* oemstring_v = lmbase + vg_aux_offset(info_p->OemStringPtr);
	char* video_v = lmbase + vg_aux_offset(info_p->VideoModePtr);

	//OEM String, null-terminated
	printf("OEM String: %s\n", oemstring_v);

	//Capabilities of the graphics environment
	printf("\nCapabilities:\n");

	if(info_p->Capabilities[0] & BIT(0)) {
		printf("DAC width is switchable to 8 bits per primary color\n");
	} else printf("DAC is fixed width, with 6 bits per primary color\n");

	if(info_p->Capabilities[0] & BIT(1)) {
		printf("Controller is not VGA compatible\n");
	} else printf("Controller is VGA compatible\n");

	if(info_p->Capabilities[0] & BIT(2)) {
		printf("When programming large blocks of information to the RAMDAC\n");
		printf("use the blank bit in Function 09h\n");
	} else printf("Normal RAMDAC operation\n");

	//VBE Modes available

	//This next section uses the VideoModePtr to find the video modes
	if(videoPtrMode == VIDEOMODEPTR || videoPtrMode == BOTH_MODES) {

		printf("\nVBE Modes available (VideoModePtr):\n");

		int lines = 0;
		i = 0;

		//Something funky here, 0x0FFFF should be the terminator but instead it's 0xFFFFFFFF, VBox's fault?
		//TODO Test this on hardware with actual VBE
		while(TRUE) {
			if (((video_v[1+i] << 8) | video_v[0+i]) == 0xFFFFFFFF) {
				break;
			} else  {
				if (lines >= LINE_BREAK) {
					printf("\n");
					printf("0x%02X ", (video_v[1+i] << 8) | (video_v[0+i]));
					lines = 0;
				} else {
					printf("0x%02X ", (video_v[1+i] << 8) | (video_v[0+i]));
				}
			}
			lines++;
			i += 2;
		}
	}

	if(videoPtrMode == BOTH_MODES)
		printf("\n");

	//This next section uses the Reserved area in the vbe_controller_info_t struct
	if(videoPtrMode == VBE_RESERVED || videoPtrMode == BOTH_MODES) {

		printf("\nVBE Modes available (Reserved Area):\n");

		int lines = 0;
		for(i = 0; i < RESERVED_SIZE - 1; i += 2) {
			if (((info_p->Reserved[1+i] << 8) | info_p->Reserved[0+i]) == 0x0FFFF) {
				break;
			} else  {
				if (lines >= LINE_BREAK) {
					printf("\n");
					printf("0x%02X ", (info_p->Reserved[1+i] << 8) | (info_p->Reserved[0+i]));
					lines = 0;
				} else {
					printf("0x%02X ", (info_p->Reserved[1+i] << 8) | (info_p->Reserved[0+i]));
				}
			}
			lines++;
		}
	}

	printf("\n\n");
	printf("Total memory: %dKB\n\n", info_p->TotalMemory * 64);

	//Start of VBE 2.0 info

	if(isVBE2) {
		//lmbase is the virtual address of physical address 0x00000000
		//sum to lmbase the physical address stored in the struct, but only
		//after converting it to a 20bit physical address
		char* oemvendor_v = lmbase + vg_aux_offset(info_p->OemVendorNamePtr);
		char* oemproductname_v = lmbase + vg_aux_offset(info_p->OemProductNamePtr);
		char* oemproductrev_v = lmbase + vg_aux_offset(info_p->OemProductRevPtr);

		//Various strings about the OEM
		printf("OEM Software Revision: %d.", ((info_p->OemSoftwareRev & 0xFF00) >> 8));
		printf("%d\n", info_p->OemSoftwareRev & 0x00FF);

		printf("OEM Vendor Name: %s\n", oemvendor_v);
		printf("OEM Product Name: %s\n", oemproductname_v);
		printf("OEM Product Revision: %s\n", oemproductrev_v);
	}

	//Free low memory block allocated previously
	lm_free(&lmblock);

	return 0;
}


//Move sprite starting from xi, yi coords. hor determines if movement is horizontal or vertical
//delta determines how many pixels the sprite should move
//time is the time the movement should take
//width, height and sprite are returns from read_xpm
int vg_move(float xi, float yi, char* sprite, unsigned short hor, short delta,
									unsigned short time, unsigned short width, unsigned short height) {

	//Subscribe timer interrupts
	int timer_hook = timer_subscribe_int();
	int kbd_hook = kbd_subscribe_int();

	//Check if subscribe worked
	if (timer_hook < 0 || kbd_hook < 0) {
		return -1;
	}

		float freq = 60; //Assuming 60Hz
		float xt = xi;
		float yt = yi;

		//Each timer interrupt takes (1.0/freq*1000) ms
		//Dividing the time passed in ms by this value we get the number of interrupts the movement will take
		float numInterrupts = (time * 1000) / (1.0/freq*1000);

		//With the number of interrupts we calculate how many pixels to move per timer interrupt
		float interruptInc = abs(delta) / numInterrupts;
		int interruptCounter = 0; //Used to check if movement is done

		if(delta < 0)
			interruptInc *= -1; //Decrement instead of increment

		//Variables for interrupt handling
		int ipc_status, dstatus;
		message msg;
		int timer_irq = BIT(timer_hook); //bitmask for timer interrupts
		int kbd_irq = BIT(kbd_hook);
		unsigned long kbd_code;

		//Interrupt handling
		while(kbd_code != ESC_BREAK) { //Repeat until ESC is released
			if ((dstatus = driver_receive(ANY, &msg, &ipc_status)) != 0) { //Receives any messages
				printf("driver_receive failed with: %d", dstatus);
				continue;
			}

			//Check if notification
			if (is_ipc_notify(ipc_status)) {
			//Debug info for notifications and interrupt bitmask
#if defined(DEBUG) && DEBUG == 1
	printf("Notification received\n");
	printf("Interrupt bitmask: 0x%02X\n\n", msg.NOTIFY_ARG);
#endif
				switch (_ENDPOINT_P(msg.m_source)) {
					case HARDWARE: //Hardware interrupt notification
						if (msg.NOTIFY_ARG & timer_irq) { //Timer interrupt

							//If limit is reached, no movement happens
							if(interruptCounter < numInterrupts) {
								if(hor == 0) {

									//Erase previous sprite
									erase_sprite(xi, yt, width, height, sprite);

									yi += interruptInc;

									//Calculate fractional part of yi
									float frac = ceil(yi) - yi;

									//This is a rough implementation of round()
									if(frac >= 0.5) {
										yt = ceil(yi);
									} else yt = floor(yi);

									//Draw sprite at new coordinates, might be the same since yt was rounded
									draw_sprite(xi, yt, width, height, sprite);

								} else {

									//Erase previous sprite
									erase_sprite(xt, yi, width, height, sprite);

									xi += interruptInc;

									//Calculate fractional part of xi
									float frac = ceil(yi) - yi;

									//This is a rough implementation of round()
									if(frac >= 0.5) {
										xt = ceil(xi);
									} else xt = floor(xi);

									//Draw sprite at new coordinates, might be the same since yt was rounded
									draw_sprite(xt, yi, width, height, sprite);
								}

								interruptCounter++;
							}
						}

						if (msg.NOTIFY_ARG & kbd_irq) { //Keyboard interrupt

							//Read output port for scancode
							kbd_code = kbd_read();

							//Check scancode read success
							if (kbd_code == KBD_ERROR) {
								kbd_reset(ENABLED);
								timer_unsubscribe_int();
								return -1;
							}
						}
						break;

					default: //No other notifications expected, do nothing
						break;
				}
			} else {
				//Debug info for standard message reception
#if defined(DEBUG) && DEBUG == 1
	printf("Standard message received\n\n");
#endif
				}
		}

		//Unsubscribe timer and keyboard interrupts
		kbd_reset(ENABLED);
		if (timer_unsubscribe_int() == 0) {
			return 0;
		} else return -1; //Failed to unsubscribe timer interrupts
}


//Free double buffer from memory
int vg_free() {
	free(double_buffer);
	return 0;
}
