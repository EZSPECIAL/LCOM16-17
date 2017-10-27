#include <minix/syslib.h>
#include "test5.h"
#include "video_gr.h"
#include "video.h"
#include "read_xpm.h"
#include "timer.h"
#include "keyboard.h"
#include "i8042.h"

//Uses the VBE INT 0x10 interface to set the desired
//graphics mode, and resets Minix's default text mode after
//a short delay. Before exiting, displays on the console the
//physical address of VRAM in the input graphics mode.
void *test_init(unsigned short mode, unsigned short delay) {

	//User sanity check
	if(delay <= 0 || delay > 10) {
		printf("vga: test_init: delay in seconds must be a value between 1 and 10\n");
		return NULL;
	}

	//Initialize h_res, v_res, bits_per_pixel for "mode"
	int phys_address = vg_init_values(mode);

	//Check if VBE Function 01h succeeded
	if(phys_address == -1) {
		return NULL;
	}

	//Call VBE Function 02h to set "mode"
	void* video_mem = vg_init(mode);

	timer_sleep(delay);

	//Return MINIX to text mode
	int timeout = TIMEOUT;
	while(timeout > 0) {
		if(vg_exit() == 0) {
			break;
		}
		timeout--;
	}

	printf("VRAM Physical Address: 0x%02X\n", phys_address);

	vg_free();
	return video_mem; //Virtual address for VRAM
}


//Tests drawing a square with the specified size and color, at the specified
//coordinates (which specify the upper left corner) in video mode 0x105
int test_square(unsigned short x, unsigned short y, unsigned short size, unsigned long color) {
	
	//Check if coordinates are inbounds, assuming mode 0x105 values
	//No need to check color, draw_pixel uses a binary & to limit color value (e.g 0xFF3D would use 0x3D if 8bpp)
	if(x < 0 || x >= H_RES || y < 0 || y >= V_RES) {
		printf("vga: test_square: either x or y are out of the screen (x [0, 1023], y [0, 767])\n");
		return -1;
	}

	//No sense in having size bigger than the screen can ever be
	if(size < 0 || size > MAXRES_105) {
		printf("vga: test_square: size too big for mode 0x105 (0 - 1024)\n");
		return -1;
	}

	//Initialize VBE mode 0x105
	if(vg_init_values(MODE105H) == -1) {
		return -1;
	}
	if(vg_init(MODE105H) == NULL) {
		return -1;
	}

	//Writes line after line of pixels to form a square
	draw_square(x, y, size, color);

	//Wait on ESC break code
	if(kbd_check_esc() != 0) {
		printf("vga: test_square: error reading keyboard scancode\n");
		return -1;
	}

	//Return MINIX to text mode
	int timeout = TIMEOUT;
	while(timeout > 0) {
		if(vg_exit() == 0) {
			break;
		}
		timeout--;
	}

	//Warn user if color wasn't on the 64 colors VBox has
	//test_square runs anyway because on native hardware palette should have 256 colors (tested on Toshiba T9000)
	if((color & 0xFF) < BLACK || (color & 0xFF) > WHITE) {
		printf("vga: test_square: warning - color specified is not defined on VBox's palette\n");
	}

	vg_free();
	return 0;
}


//Tests drawing a line segment with the specified start and end points and the input color,
//by writing to VRAM in video mode 0x105
int test_line(unsigned short xi, unsigned short yi, 
				unsigned short xf, unsigned short yf, unsigned long color) {
	
	//Check if coordinates are inbounds, assuming mode 0x105 values
	if(xi < 0 || xi >= H_RES || xf < 0 || xf >= H_RES || yi < 0 || yi >= V_RES || yf < 0 || yf >= V_RES) {
		printf("vga: test_line: one or more of the points is out of the screen (x [0, 1023], y [0, 767])\n");
		return -1;
	}

	//Initialize VBE mode 0x105
	if(vg_init_values(MODE105H) == -1) {
		return -1;
	}
	if(vg_init(MODE105H) == NULL) {
		return -1;
	}

	//Draw line using Bresenham's Line Algorithm, adapted to work in all octants and vertically/horizontally
	draw_line(xi, yi, xf, yf, color);

	//Wait on ESC break code
	if(kbd_check_esc() != 0) {
		printf("vga: test_line: error reading keyboard scancode\n");
		return -1;
	}

	//Return MINIX to text mode
	int timeout = TIMEOUT;
	while(timeout > 0) {
		if(vg_exit() == 0) {
			break;
		}
		timeout--;
	}

	//Warn user if color wasn't on the 64 colors VBox has
	//test_line runs anyway because on native hardware palette should have 256 colors (tested on Toshiba T9000)
	if((color & 0xFF) < BLACK || (color & 0xFF) > WHITE) {
		printf("vga: test_line: warning - color specified is not defined on VBox's palette\n");
	}

	vg_free();
	return 0;
}


//Tests drawing a sprite from an XPM on the screen at specified coordinates
//by writing to VRAM in video mode 0x105
int test_xpm(unsigned short xi, unsigned short yi, char *xpm[]) {
	
	//Check if coordinates are inbounds, assuming mode 0x105 values
	if(xi < 0 || xi >= H_RES || yi < 0 || yi >= V_RES) {
		printf("vga: test_xpm: either x or y are out of the screen (x [0, 1023], y [0, 767])\n");
		return -1;
	}

	//Initialize VBE mode 0x105
	if(vg_init_values(MODE105H) == -1) {
		return -1;
	}
	if(vg_init(MODE105H) == NULL) {
		return -1;
	}

	int w, h;
	char* sprite;

	//Loads xpm to sprite array and initializes width and height values
	sprite = read_xpm(xpm, &w, &h);

	if(sprite == NULL) {
		printf("vga: test_xpm: failed to load XPM to sprite array\n");
		return -1;
	}

	//Auxiliary function to draw sprite
	draw_sprite(xi, yi, w, h, sprite);

	//Wait on ESC break code
	if(kbd_check_esc() != 0) {
		printf("vga: test_xpm: error reading keyboard scancode\n");
		return -1;
	}

	//Return MINIX to text mode and free memory
	int timeout = TIMEOUT;
	while(timeout > 0) {
		if(vg_exit() == 0) {
			break;
		}
		timeout--;
	}

	free(sprite);
	vg_free();
	return 0;
}


//Tests moving a XPM on the screen along horizontal/vertical axes, at the specified speed, in video mode 0x105
int test_move(unsigned short xi, unsigned short yi, char *xpm[],
					unsigned short hor, short delta, unsigned short time) {

	//Check if coordinates are inbounds, assuming mode 0x105 values
	if(xi < 0 || xi >= H_RES || yi < 0 || yi >= V_RES) {
		printf("vga: test_move: either x or y are out of the screen (x [0, 1023], y [0, 767])\n");
		return -1;
	}

	//User sanity check
	if(time <= 0 || time > 20) {
		printf("vga: test_move: delay in seconds must be a value between 1 and 20\n");
		return -1;
	}

	if(delta == 0 || abs(delta) > MAXRES_105) {
		printf("vga: test_move: delta has to be a value from 1 to 1024\n");
		return -1;
	}

	//Initialize VBE mode 0x105
	if(vg_init_values(MODE105H) == -1) {
		return -1;
	}
	if(vg_init(MODE105H) == NULL) {
		return -1;
	}

	int w, h;
	char* sprite;

	//Loads xpm to sprite array and initializes width and height values
	sprite = read_xpm(xpm, &w, &h);

	if(sprite == NULL) {
		printf("vga: test_move: failed to load XPM to sprite array\n");
		return -1;
	}

	if(vg_move(xi, yi, sprite, hor, delta, time, w, h) != 0) {
		return -1; //Functions inside vg_move() already print errors
	}

	//Return MINIX to text mode and free memory
	int timeout = TIMEOUT;
	while(timeout > 0) {
		if(vg_exit() == 0) {
			break;
		}
		timeout--;
	}

	free(sprite);
	vg_free();
	return 0;
}

//Tests retrieving VBE controller information (VBE function 00h) and its parsing
int test_controller() {

	//Let user choose how test_controller behaves
	int choice = kbd_controller_choice();

	if(choice == -1) {
		printf("vga: test_controller: error choosing option\n");
		return -1;
	}

	//Info about vg_controller on video_gr.c
	//TRUE presets VBE2, FALSE doesn't
	if(vg_controller(TRUE, choice) != 0) {
		return -1;
	}

	return 0;
}
