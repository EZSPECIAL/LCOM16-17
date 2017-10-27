#include <minix/syslib.h>
#include <minix/drivers.h>
#include <machine/int86.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <math.h>
#include <stdint.h>

#include "lmlib.h"
#include "vbe.h"
#include "video_gr.h"
#include "video.h"
#include "logic.h"
#include "LoLCOM.h"

//Variables whose scope is video_gr.c

static phys_bytes video_phys;	//Physical address for VRAM
								//vg_init takes "mode" and returns virtual address of
								//VRAM, since it came already declared, had to create
								//auxiliary function to return this value and set it
								//so vg_init can use it later

static char *video_mem;			//Virtual address for VRAM
static char *double_buffer;		//Buffer to use for double buffering
static uint8_t vram_page = 0;
static uint8_t use_double_buffer = TRUE;

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

	int vram_size;

	vram_size = 2 * h_res * v_res * (bits_per_pixel / 8);

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

	if(x < h_res && y < v_res && color != TRANSPARENT) {

		char* vram_t;

		//draw_pixel writes to double buffer or page outside of view
		if(use_double_buffer == TRUE) {
			vram_t = double_buffer;
		} else {
			if(vram_page == 0) {
				vram_t = video_mem + h_res * v_res * (bits_per_pixel / 8);
			} else {
				vram_t = video_mem;
			}
		}

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


int8_t vg_tile(point_t coords, uint16_t tilesheet_width, uint8_t tilesperline, unsigned char* tileset, uint8_t tilenumber) {

	unsigned short tilex = (tilenumber % tilesperline) * TILESIZE;
	unsigned short tiley = (tilenumber / tilesperline) * TILESIZE;

	int i, j;
	for (i = 0; i < TILESIZE; i++) {
		for (j = 0; j < TILESIZE; j++) {
			unsigned long r = tileset[(tilex + j + (tiley + i) * tilesheet_width) * COMPONENTS];
			unsigned long g = tileset[(tilex + j + (tiley + i) * tilesheet_width) * COMPONENTS + 1];
			unsigned long b = tileset[(tilex + j + (tiley + i) * tilesheet_width) * COMPONENTS + 2];
			unsigned long color = ((r << 16) & 0x00FF0000) | ((g << 8) & 0x0000FF00) | (b & 0x000000FF);
			draw_pixel(coords.x + j, coords.y + i, color);
		}
	}

	return 0;
}


int8_t vg_font(point_t coords, uint16_t fontdata_width, uint8_t tilesperline, unsigned char* fontdata, uint8_t tilenumber) {

	unsigned short tilex = (tilenumber % tilesperline) * FONT_W;
	unsigned short tiley = (tilenumber / tilesperline) * FONT_H;

	int i, j;
	for (i = 0; i < FONT_H; i++) {
		for (j = 0; j < FONT_W; j++) {
			unsigned long r = fontdata[(tilex + j + (tiley + i) * fontdata_width) * COMPONENTS];
			unsigned long g = fontdata[(tilex + j + (tiley + i) * fontdata_width) * COMPONENTS + 1];
			unsigned long b = fontdata[(tilex + j + (tiley + i) * fontdata_width) * COMPONENTS + 2];
			unsigned long color = ((r << 16) & 0x00FF0000) | ((g << 8) & 0x0000FF00) | (b & 0x000000FF);
			draw_pixel(coords.x + j, coords.y + i, color);
		}
	}

	return 0;
}


int8_t vg_draw_map(point_t coords) {

	point_t tile_coords;
	map_t currentmap = map_get();

	int i, j;
	int currLine = 0;
	for (i = 0; i < MHEIGHT; i++) {
		for (j = 0; j < MWIDTH; j++) {
			tile_coords.x = coords.x + j * TILESIZE;
			tile_coords.y = coords.y + i * TILESIZE;
			vg_tile(tile_coords, currentmap.tileset_width, currentmap.tilesperline, currentmap.tileset, currentmap.map[j + i*MWIDTH]);
		}
		currLine++;
	}

	return 0;
}


int8_t vg_png(point_t coords, uint16_t image_width, uint16_t image_height, unsigned char* image) {

	int i, j;
	for (i = 0; i < image_height; i++) {
		for (j = 0; j < image_width; j++) {
			unsigned long r = image[(j + i * image_width) * COMPONENTS];
			unsigned long g = image[(j + i * image_width) * COMPONENTS + 1];
			unsigned long b = image[(j + i * image_width) * COMPONENTS + 2];
			unsigned long color = ((r << 16) & 0x00FF0000) | ((g << 8) & 0x0000FF00) | (b & 0x000000FF);
			draw_pixel(coords.x + j, coords.y + i, color);
		}
	}

	return 0;
}


int vg_clear() {

	if(use_double_buffer == TRUE) {
		memset(double_buffer, BLACK, h_res * v_res * (bits_per_pixel / 8));
	} else {
		char* vram_t;

		if(vram_page == 0) {
			vram_t = video_mem + h_res * v_res * (bits_per_pixel / 8);
		} else {
			vram_t = video_mem;
		}

		memset(vram_t, BLACK, h_res * v_res * (bits_per_pixel / 8));
	}

	return 0;
}


//Free double buffer from memory
int vg_free() {

	free(double_buffer);
	return 0;
}


int8_t vg_refresh() {

	if(use_double_buffer == FALSE)  {
		vram_page ^= BIT(0);
		vg_pageflip();
	} else {
		memcpy(video_mem, double_buffer, h_res * v_res * (bits_per_pixel / 8));
	}

	return 0;
}


void vg_topleft(uint16_t* topleft_x, uint16_t* topleft_y) {
	*topleft_x = (h_res - 16*TILESIZE) / 2;
	*topleft_y = (v_res - 15*TILESIZE) / 2;
}


int8_t vg_pageflip() {

	uint32_t offset = vram_page * v_res;

	struct reg86u reg86;

	reg86.u.b.intno = BIOS_VIDEO;
	reg86.u.w.ax = SET_DISPLAY_START;
	reg86.u.b.bh = 0x00;
	reg86.u.b.bl = 0x00;
	reg86.u.w.cx = 0x00;
	reg86.u.w.dx = offset;

	if( sys_int86(&reg86) != OK ) {
		printf("vga: vg_pageflip: sys_int86 call failed\n");
		return -1;
	}

	//Check function support in AL register
	if(reg86.u.b.al != FSUPPORTED) {
		printf("vga: vg_pageflip: mode is not supported\n");
		return -1;
	}

	//Check function call status in AH register
	if(reg86.u.b.ah != FCALL_SUCCESS) {

		switch(reg86.u.b.ah) {
		case FCALL_FAIL:
			printf("vga: vg_pageflip: function call failed\n");
			break;
		case FHARDWARE:
			printf("vga: vg_pageflip: function call is not supported in current hardware configuration\n");
			break;
		case FINVALID:
			printf("vga: vg_pageflip: function call invalid in current video mode\n");
			break;
		default:
			printf("vga: vg_pageflip: unknown VBE function error\n");
			break;
		}

		return -1;
	}

	return 0;
}

void vg_change_buffering(uint8_t mode) {

	if(mode == DOUBLEBUFFER) {
		use_double_buffer = TRUE;
		vram_page = 0;
		vg_pageflip();
	} else if(mode == PAGEFLIP) {
		use_double_buffer = FALSE;
	}
}
